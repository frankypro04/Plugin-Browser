#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::File appDir()
{
    auto d = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("PluginBrowser");
    d.createDirectory();
    return d;
}

//==============================================================================
class PluginBrowserProcessor::ScanThread : public juce::Thread
{
public:
    explicit ScanThread (PluginBrowserProcessor& o) : juce::Thread ("PluginScan"), owner (o) {}

    void run() override
    {
        juce::VST3PluginFormat format;
        juce::PluginDirectoryScanner scanner (owner.knownPlugins, format,
                                              format.getDefaultLocationsToSearch(), true,
                                              appDir().getChildFile ("scan_crash.txt"), true);
        juce::String name;
        while (! threadShouldExit() && scanner.scanNextFile (true, name))
        {
            owner.scanProgress = scanner.getProgress();
            const juce::ScopedLock sl (owner.statusLock);
            owner.scanStatus = name;
        }
        owner.scanProgress = 1.0f;
        owner.saveKnownPlugins();
        owner.scanning = false;
    }

private:
    PluginBrowserProcessor& owner;
};

//==============================================================================
PluginBrowserProcessor::PluginBrowserProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    formatManager.addDefaultFormats();

    if (auto xml = juce::parseXML (appDir().getChildFile ("plugins.xml")))
        knownPlugins.recreateFromXml (*xml);
}

PluginBrowserProcessor::~PluginBrowserProcessor()
{
    if (scanThread != nullptr)
        scanThread->stopThread (5000);
}

void PluginBrowserProcessor::saveKnownPlugins()
{
    if (auto xml = knownPlugins.createXml())
        xml->writeTo (appDir().getChildFile ("plugins.xml"));
}

void PluginBrowserProcessor::startScan (bool rescanEverything)
{
    if (scanning.load())
        return;

    if (scanThread != nullptr)
        scanThread->stopThread (2000);

    if (rescanEverything)
        knownPlugins.clear();

    scanning = true;
    scanProgress = 0.0f;
    scanThread = std::make_unique<ScanThread> (*this);
    scanThread->startThread (juce::Thread::Priority::background);
}

juce::String PluginBrowserProcessor::getScanStatus() const
{
    const juce::ScopedLock sl (statusLock);
    return scanStatus;
}

//==============================================================================
void PluginBrowserProcessor::prepareToPlay (double sr, int bs)
{
    currentRate = sr;
    currentBlock = bs;
    scratch.setSize (8, bs);

    const juce::ScopedLock sl (hostLock);
    if (hosted != nullptr)
    {
        hosted->setRateAndBufferSizeDetails (sr, bs);
        hosted->prepareToPlay (sr, bs);
    }
}

bool PluginBrowserProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono())
           && out == l.getMainInputChannelSet();
}

void PluginBrowserProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const juce::ScopedTryLock sl (hostLock);
    if (! sl.isLocked() || hosted == nullptr)
        return; // sin plugin cargado: el audio pasa sin cambios

    const int n = buffer.getNumSamples();
    if (n > scratch.getNumSamples())
        return;

    const int chans = juce::jmin (buffer.getNumChannels(), scratch.getNumChannels());
    scratch.clear();
    for (int ch = 0; ch < chans; ++ch)
        scratch.copyFrom (ch, 0, buffer, ch, 0, n);

    juce::AudioBuffer<float> view (scratch.getArrayOfWritePointers(), scratch.getNumChannels(), n);
    hosted->processBlock (view, midi);

    for (int ch = 0; ch < chans; ++ch)
        buffer.copyFrom (ch, 0, view, ch, 0, n);
}

//==============================================================================
void PluginBrowserProcessor::loadPlugin (const juce::PluginDescription& d, juce::MemoryBlock state)
{
    lastError.clear();
    juce::WeakReference<PluginBrowserProcessor> weak (this);

    formatManager.createPluginInstanceAsync (d, currentRate, currentBlock,
        [weak, state] (std::unique_ptr<juce::AudioPluginInstance> inst, const juce::String& error) mutable
        {
            if (weak != nullptr)
                weak->finishLoading (std::move (inst), error, state);
        });
}

void PluginBrowserProcessor::finishLoading (std::unique_ptr<juce::AudioPluginInstance> inst,
                                            const juce::String& error, juce::MemoryBlock state)
{
    if (inst == nullptr)
    {
        lastError = error.isEmpty() ? "No se pudo cargar el plugin" : error;
        listeners.call ([] (Listener& l) { l.hostedChanged(); });
        return;
    }

    // Estéreo en el bus principal, buses extra (sidechain) desactivados
    BusesLayout layout;
    for (int i = 0; i < inst->getBusCount (true); ++i)
        layout.inputBuses.add (i == 0 ? juce::AudioChannelSet::stereo() : juce::AudioChannelSet::disabled());
    for (int i = 0; i < inst->getBusCount (false); ++i)
        layout.outputBuses.add (i == 0 ? juce::AudioChannelSet::stereo() : juce::AudioChannelSet::disabled());
    inst->setBusesLayout (layout);

    if (state.getSize() > 0)
        inst->setStateInformation (state.getData(), (int) state.getSize());

    inst->setRateAndBufferSizeDetails (currentRate, currentBlock);
    inst->prepareToPlay (currentRate, currentBlock);

    listeners.call ([] (Listener& l) { l.hostedAboutToChange(); });

    std::unique_ptr<juce::AudioPluginInstance> old;
    {
        const juce::ScopedLock sl (hostLock);
        old = std::move (hosted);
        hosted = std::move (inst);
    }
    old.reset();

    setLatencySamples (hosted->getLatencySamples());
    listeners.call ([] (Listener& l) { l.hostedChanged(); });
}

void PluginBrowserProcessor::unloadPlugin()
{
    listeners.call ([] (Listener& l) { l.hostedAboutToChange(); });

    std::unique_ptr<juce::AudioPluginInstance> old;
    {
        const juce::ScopedLock sl (hostLock);
        old = std::move (hosted);
    }
    old.reset();
    setLatencySamples (0);
    listeners.call ([] (Listener& l) { l.hostedChanged(); });
}

//==============================================================================
void PluginBrowserProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::XmlElement xml ("PLUGINBROWSER");
    xml.setAttribute ("w", browserW);
    xml.setAttribute ("h", browserH);
    xml.setAttribute ("scale", (double) uiScale);
    xml.setAttribute ("group", groupByMaker);

    if (hosted != nullptr)
    {
        xml.addChildElement (hosted->getPluginDescription().createXml().release());
        juce::MemoryBlock m;
        hosted->getStateInformation (m);
        xml.createNewChildElement ("STATE")->addTextElement (m.toBase64Encoding());
    }
    copyXmlToBinary (xml, dest);
}

void PluginBrowserProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName ("PLUGINBROWSER"))
        return;

    browserW = xml->getIntAttribute ("w", 860);
    browserH = xml->getIntAttribute ("h", 620);
    uiScale = (float) xml->getDoubleAttribute ("scale", 1.0);
    groupByMaker = xml->getBoolAttribute ("group", true);

    if (auto* d = xml->getChildByName ("PLUGIN"))
    {
        juce::PluginDescription desc;
        if (desc.loadFromXml (*d))
        {
            juce::MemoryBlock st;
            if (auto* s = xml->getChildByName ("STATE"))
                st.fromBase64Encoding (s->getAllSubText());

            juce::WeakReference<PluginBrowserProcessor> weak (this);
            juce::MessageManager::callAsync ([weak, desc, st]
            {
                if (weak != nullptr)
                    weak->loadPlugin (desc, st);
            });
        }
    }
}

juce::AudioProcessorEditor* PluginBrowserProcessor::createEditor()
{
    return new PluginBrowserEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginBrowserProcessor();
}
