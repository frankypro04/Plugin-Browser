#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_processors/juce_audio_processors.h>

class PluginBrowserProcessor : public juce::AudioProcessor
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void hostedAboutToChange() {}
        virtual void hostedChanged() {}
    };

    PluginBrowserProcessor();
    ~PluginBrowserProcessor() override;

    // ---- AudioProcessor
    void prepareToPlay (double sampleRate, int blockSize) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Plugin Browser"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- Hosting
    void loadPlugin (const juce::PluginDescription&, juce::MemoryBlock state = {});
    void unloadPlugin();
    juce::AudioPluginInstance* getHosted() const { return hosted.get(); }
    juce::String getLastError() const { return lastError; }

    // ---- Scanning
    juce::KnownPluginList& getKnownPlugins() { return knownPlugins; }
    void startScan (bool rescanEverything);
    bool isScanning() const { return scanning.load(); }
    float getScanProgress() const { return scanProgress.load(); }
    juce::String getScanStatus() const;

    void addListener (Listener* l)    { listeners.add (l); }
    void removeListener (Listener* l) { listeners.remove (l); }

    // UI state saved with the project
    int browserW = 860, browserH = 620;
    float uiScale = 1.0f;
    bool groupByMaker = true;

private:
    class ScanThread;
    void finishLoading (std::unique_ptr<juce::AudioPluginInstance>, const juce::String& error, juce::MemoryBlock state);
    void saveKnownPlugins();

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;

    std::unique_ptr<juce::AudioPluginInstance> hosted;
    juce::CriticalSection hostLock;
    juce::AudioBuffer<float> scratch;
    double currentRate = 44100.0;
    int currentBlock = 512;
    juce::String lastError;

    std::unique_ptr<ScanThread> scanThread;
    std::atomic<bool> scanning { false };
    std::atomic<float> scanProgress { 0.0f };
    juce::String scanStatus;
    mutable juce::CriticalSection statusLock;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_WEAK_REFERENCEABLE (PluginBrowserProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginBrowserProcessor)
};
