#include "PluginEditor.h"

using namespace Palette;

//==============================================================================
BrowserLookAndFeel::BrowserLookAndFeel()
{
    setColour (juce::TextButton::buttonColourId, row);
    setColour (juce::TextButton::buttonOnColourId, accent);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::TextEditor::backgroundColourId, panel);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, juce::Colours::white.withAlpha (0.08f));
    setColour (juce::TextEditor::focusedOutlineColourId, accent);
    setColour (juce::CaretComponent::caretColourId, accent2);
    setColour (juce::ComboBox::backgroundColourId, row);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha (0.08f));
    setColour (juce::ComboBox::arrowColourId, dim);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour (juce::ScrollBar::thumbColourId, juce::Colours::white.withAlpha (0.18f));
    setColour (juce::ProgressBar::backgroundColourId, row);
    setColour (juce::ProgressBar::foregroundColourId, accent2);
    setColour (juce::Label::textColourId, text);
}

void BrowserLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (on ? accent : (down ? rowHover.brighter (0.1f) : over ? rowHover : row));
    g.fillRoundedRectangle (r, 9.0f);
    g.setColour (on ? accent.brighter (0.3f) : juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (r, 9.0f, 1.0f);
}

void BrowserLookAndFeel::drawTreeviewPlusMinusBox (juce::Graphics& g, const juce::Rectangle<float>& area,
                                                   juce::Colour, bool isOpen, bool over)
{
    juce::Path p;
    auto c = area.getCentre();
    if (isOpen) p.addTriangle (c.x - 4, c.y - 2, c.x + 4, c.y - 2, c.x, c.y + 3);
    else        p.addTriangle (c.x - 2, c.y - 4, c.x - 2, c.y + 4, c.x + 3, c.y);
    g.setColour (over ? accent2 : dim);
    g.fillPath (p);
}

//==============================================================================
using LoadFn = std::function<void (const juce::PluginDescription&)>;

class PluginRow : public juce::TreeViewItem
{
public:
    PluginRow (const juce::PluginDescription& d, bool showMaker, LoadFn fn)
        : desc (d), maker (showMaker), load (std::move (fn)) {}

    bool mightContainSubItems() override { return false; }
    juce::String getUniqueName() const override { return desc.createIdentifierString(); }
    int getItemHeight() const override { return 40; }

    void paintItem (juce::Graphics& g, int w, int h) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (4.0f, 2.0f);
        g.setColour (isSelected() ? accent.withAlpha (0.30f) : row);
        g.fillRoundedRectangle (r, 9.0f);
        if (isSelected())
        {
            g.setColour (accent);
            g.drawRoundedRectangle (r, 9.0f, 1.2f);
        }

        const auto dot = desc.isInstrument ? accent2 : accent;
        g.setColour (dot);
        g.fillEllipse (r.getX() + 14, r.getCentreY() - 4.5f, 9.0f, 9.0f);

        auto badge = r.removeFromRight (86).reduced (6, 9);
        g.setColour (dot.withAlpha (0.16f));
        g.fillRoundedRectangle (badge, 6.0f);
        g.setColour (dot);
        g.setFont (uiFont (11.5f, juce::Font::bold));
        g.drawText (desc.isInstrument ? "INSTRUMENTO" : "EFECTO", badge.toNearestInt(), juce::Justification::centred);

        auto textArea = r.withTrimmedLeft (34).toNearestInt();
        g.setColour (text);
        g.setFont (uiFont (15.0f, juce::Font::bold));
        g.drawText (desc.name, textArea.removeFromLeft (juce::jmax (120, textArea.getWidth() / 2)),
                    juce::Justification::centredLeft, true);

        if (maker)
        {
            g.setColour (dim);
            g.setFont (uiFont (13.0f));
            g.drawText (desc.manufacturerName, textArea, juce::Justification::centredLeft, true);
        }
    }

    void itemDoubleClicked (const juce::MouseEvent&) override { load (desc); }

private:
    juce::PluginDescription desc;
    bool maker;
    LoadFn load;
};

class MakerRow : public juce::TreeViewItem
{
public:
    MakerRow (juce::String n) : name (std::move (n)) {}

    bool mightContainSubItems() override { return true; }
    juce::String getUniqueName() const override { return "maker:" + name; }
    int getItemHeight() const override { return 34; }
    void itemClicked (const juce::MouseEvent&) override { setOpen (! isOpen()); }

    void paintItem (juce::Graphics& g, int w, int h) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (4.0f, 2.0f);
        g.setColour (panel);
        g.fillRoundedRectangle (r, 9.0f);

        g.setColour (text);
        g.setFont (uiFont (14.5f, juce::Font::bold));
        g.drawText (name.toUpperCase(), r.withTrimmedLeft (28).toNearestInt(), juce::Justification::centredLeft, true);

        auto badge = r.removeFromRight (44).reduced (6, 8);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRoundedRectangle (badge, 6.0f);
        g.setColour (dim);
        g.setFont (uiFont (12.0f, juce::Font::bold));
        g.drawText (juce::String (getNumSubItems()), badge.toNearestInt(), juce::Justification::centred);
    }

private:
    juce::String name;
};

class RootRow : public juce::TreeViewItem
{
public:
    bool mightContainSubItems() override { return true; }
};

//==============================================================================
PluginBrowserEditor::PluginBrowserEditor (PluginBrowserProcessor& p)
    : juce::AudioProcessorEditor (p), processor (p)
{
    setLookAndFeel (&laf);

    for (auto* c : std::initializer_list<juce::Component*> { &search, &byMakerBtn, &listBtn, &rescanBtn, &kindBox,
                                                              &status, &progressBar, &tree,
                                                              &backBtn, &unloadBtn, &scaleBox, &hostedName })
        addAndMakeVisible (c);

    search.setTextToShowWhenEmpty ("Buscar plugin, fabricante o categoria...", dim);
    search.setFont (uiFont (15.0f));
    search.setIndents (14, 0);
    search.onTextChange = [this] { rebuildTree(); };

    byMakerBtn.setClickingTogglesState (true);
    listBtn.setClickingTogglesState (true);
    byMakerBtn.setRadioGroupId (11);
    listBtn.setRadioGroupId (11);
    (processor.groupByMaker ? byMakerBtn : listBtn).setToggleState (true, juce::dontSendNotification);
    byMakerBtn.onClick = [this] { processor.groupByMaker = true;  rebuildTree(); };
    listBtn.onClick    = [this] { processor.groupByMaker = false; rebuildTree(); };

    kindBox.addItemList ({ "Todos", "Instrumentos", "Efectos" }, 1);
    kindBox.setSelectedId (1, juce::dontSendNotification);
    kindBox.onChange = [this] { rebuildTree(); };

    rescanBtn.setTooltip ("Shift + clic: re-escanear todo desde cero");
    rescanBtn.onClick = [this] { processor.startScan (juce::ModifierKeys::currentModifiers.isShiftDown()); };

    status.setFont (uiFont (12.5f));
    status.setColour (juce::Label::textColourId, dim);
    progressBar.setPercentageDisplay (false);

    tree.setRootItemVisible (false);
    tree.setDefaultOpenness (false);
    tree.setIndentSize (16);
    tree.setColour (juce::TreeView::backgroundColourId, juce::Colours::transparentBlack);

    backBtn.onClick = [this] { showPluginView (false); };
    unloadBtn.onClick = [this] { processor.unloadPlugin(); };

    scaleBox.addItemList ({ "50%", "75%", "100%", "125%", "150%", "200%" }, 1);
    const float scales[] = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
    int sel = 3;
    for (int i = 0; i < 6; ++i)
        if (juce::approximatelyEqual (scales[i], processor.uiScale)) sel = i + 1;
    scaleBox.setSelectedId (sel, juce::dontSendNotification);
    scaleBox.setTooltip ("Escala del plugin (depende de que el plugin la soporte)");
    scaleBox.onChange = [this] { applyScale(); };

    hostedName.setFont (uiFont (15.0f, juce::Font::bold));

    setResizable (true, true);
    setResizeLimits (620, 420, 4096, 4096);
    setSize (processor.browserW, processor.browserH);

    processor.getKnownPlugins().addChangeListener (this);
    processor.addListener (this);

    if (processor.getHosted() != nullptr)
    {
        createHostedEditor();
        showPluginView (true);
    }
    else
    {
        showPluginView (false);
    }

    rebuildTree();
    if (processor.getKnownPlugins().getNumTypes() == 0)
        processor.startScan (false);

    startTimerHz (4);
}

PluginBrowserEditor::~PluginBrowserEditor()
{
    stopTimer();
    processor.removeListener (this);
    processor.getKnownPlugins().removeChangeListener (this);
    destroyHostedEditor();
    tree.setRootItem (nullptr);
    setLookAndFeel (nullptr);
}

//==============================================================================
void PluginBrowserEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (bg1, 0, 0, bg0, 0, (float) getHeight(), false));
    g.fillAll();

    auto top = getLocalBounds().removeFromTop (topH);
    g.setColour (panel.withAlpha (0.92f));
    g.fillRect (top);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine (topH - 1, 0.0f, (float) getWidth());

    // Icono
    auto icon = juce::Rectangle<float> (18, 14, 28, 28);
    g.setGradientFill (juce::ColourGradient (accent, icon.getX(), icon.getY(),
                                             accent2, icon.getRight(), icon.getBottom(), false));
    g.fillRoundedRectangle (icon, 8.0f);
    g.setColour (juce::Colours::white);
    g.drawEllipse (icon.getX() + 6, icon.getY() + 6, 11, 11, 2.0f);
    g.drawLine (icon.getX() + 15.5f, icon.getY() + 15.5f, icon.getX() + 22, icon.getY() + 22, 2.2f);

    if (! pluginView)
    {
        g.setColour (text);
        g.setFont (uiFont (20.0f, juce::Font::bold));
        g.drawText ("Plugin Browser", 58, 0, 260, topH, juce::Justification::centredLeft);
    }
}

void PluginBrowserEditor::resized()
{
    auto r = getLocalBounds();
    auto top = r.removeFromTop (topH).reduced (12, 10);

    if (pluginView)
    {
        top.removeFromLeft (40);
        backBtn.setBounds (top.removeFromLeft (130));
        unloadBtn.setBounds (top.removeFromRight (84));
        top.removeFromRight (8);
        scaleBox.setBounds (top.removeFromRight (84));
        top.removeFromLeft (12);
        hostedName.setBounds (top);

        if (hostedEditor != nullptr)
        {
            const juce::ScopedValueSetter<bool> guard (ignoreHostedResize, true);
            if (hostedEditor->isResizable())
                hostedEditor->setBounds (r);
            else
                hostedEditor->setTopLeftPosition (r.getX() + juce::jmax (0, (r.getWidth() - hostedEditor->getWidth()) / 2), r.getY());
        }
        return;
    }

    processor.browserW = getWidth();
    processor.browserH = getHeight();

    rescanBtn.setBounds (top.removeFromRight (110));

    auto bar = r.reduced (14, 10).removeFromTop (40);
    kindBox.setBounds (bar.removeFromRight (130));
    bar.removeFromRight (8);
    listBtn.setBounds (bar.removeFromRight (70));
    bar.removeFromRight (6);
    byMakerBtn.setBounds (bar.removeFromRight (130));
    bar.removeFromRight (8);
    search.setBounds (bar);

    auto bottom = r.removeFromBottom (30).reduced (16, 4);
    progressBar.setBounds (bottom.removeFromRight (180).reduced (0, 3));
    status.setBounds (bottom);

    r.removeFromTop (60);
    tree.setBounds (r.reduced (10, 0));
}

//==============================================================================
void PluginBrowserEditor::rebuildTree()
{
    const auto q = search.getText().trim().toLowerCase();
    const int kind = kindBox.getSelectedId();

    juce::Array<juce::PluginDescription> list;
    for (auto& d : processor.getKnownPlugins().getTypes())
    {
        if (d.name == "Plugin Browser") continue;
        if (kind == 2 && ! d.isInstrument) continue;
        if (kind == 3 && d.isInstrument) continue;
        if (q.isNotEmpty() && ! (d.name + " " + d.manufacturerName + " " + d.category).toLowerCase().contains (q))
            continue;
        list.add (d);
    }
    std::sort (list.begin(), list.end(), [] (const auto& a, const auto& b)
               { return a.name.compareIgnoreCase (b.name) < 0; });

    std::unique_ptr<juce::XmlElement> openState;
    if (root != nullptr)
        openState.reset (tree.getOpennessState (true));
    tree.setRootItem (nullptr);

    root = std::make_unique<RootRow>();
    LoadFn load = [this] (const juce::PluginDescription& d)
    {
        status.setText ("Cargando " + d.name + "...", juce::dontSendNotification);
        processor.loadPlugin (d);
    };

    int makerCount = 0;
    if (processor.groupByMaker)
    {
        juce::StringArray makers;
        for (auto& d : list)
            makers.addIfNotAlreadyThere (d.manufacturerName.isEmpty() ? "Desconocido" : d.manufacturerName);
        makers.sort (true);
        makerCount = makers.size();

        for (auto& m : makers)
        {
            auto* item = new MakerRow (m);
            for (auto& d : list)
                if ((d.manufacturerName.isEmpty() ? juce::String ("Desconocido") : d.manufacturerName) == m)
                    item->addSubItem (new PluginRow (d, false, load));
            root->addSubItem (item);
        }
    }
    else
    {
        for (auto& d : list)
            root->addSubItem (new PluginRow (d, true, load));
    }

    tree.setRootItem (root.get());

    if (q.isNotEmpty())
    {
        for (int i = 0; i < root->getNumSubItems(); ++i)
            root->getSubItem (i)->setOpen (true);
    }
    else if (openState != nullptr)
    {
        tree.restoreOpennessState (*openState, true);
    }

    if (! processor.isScanning())
        status.setText (juce::String (list.size()) + " plugins" + (processor.groupByMaker ? " · " + juce::String (makerCount) + " fabricantes" : juce::String()),
                        juce::dontSendNotification);
}

void PluginBrowserEditor::timerCallback()
{
    const bool sc = processor.isScanning();
    progressBar.setVisible (sc && ! pluginView);
    if (sc)
    {
        progress = processor.getScanProgress();
        status.setText ("Escaneando: " + processor.getScanStatus(), juce::dontSendNotification);
    }

    if (listDirty)
    {
        listDirty = false;
        rebuildTree();
    }
    else if (! sc && status.getText().startsWith ("Escaneando"))
    {
        rebuildTree();
    }

    if (! pluginView && ! sc && processor.getHosted() == nullptr && processor.getLastError().isNotEmpty())
        status.setText ("Error: " + processor.getLastError(), juce::dontSendNotification);
}

//==============================================================================
void PluginBrowserEditor::showPluginView (bool show)
{
    pluginView = show;

    for (auto* c : std::initializer_list<juce::Component*> { &search, &byMakerBtn, &listBtn, &rescanBtn,
                                                             &kindBox, &status, &progressBar, &tree })
        c->setVisible (! show);

    for (auto* c : std::initializer_list<juce::Component*> { &backBtn, &unloadBtn, &scaleBox, &hostedName })
        c->setVisible (show);

    if (hostedEditor != nullptr)
        hostedEditor->setVisible (show);

    if (show)
    {
        fitToHosted();
    }
    else
    {
        setResizeLimits (620, 420, 4096, 4096);
        setSize (processor.browserW, processor.browserH);
    }

    resized();
    repaint();
}

void PluginBrowserEditor::createHostedEditor()
{
    destroyHostedEditor();
    auto* h = processor.getHosted();
    if (h == nullptr)
        return;

    if (h->hasEditor())
        hostedEditor.reset (h->createEditorIfNeeded());
    if (hostedEditor == nullptr)
        hostedEditor = std::make_unique<juce::GenericAudioProcessorEditor> (*h);

    addChildComponent (*hostedEditor);
    hostedEditor->addComponentListener (this);
    hostedEditor->setScaleFactor (processor.uiScale);

    hostedName.setText (h->getName() + "   ·   " + h->getPluginDescription().manufacturerName,
                        juce::dontSendNotification);
}

void PluginBrowserEditor::destroyHostedEditor()
{
    if (hostedEditor != nullptr)
    {
        hostedEditor->removeComponentListener (this);
        removeChildComponent (hostedEditor.get());
        hostedEditor.reset();
    }
}

void PluginBrowserEditor::fitToHosted()
{
    if (hostedEditor == nullptr)
        return;

    const auto b = hostedEditor->getBoundsInParent();
    const int W = juce::jmax (b.getWidth(), 560);
    const int H = b.getHeight() + topH;

    if (hostedEditor->isResizable())
        setResizeLimits (560, 320, 8192, 8192);   // tamano libre: arrastra la esquina
    else
        setResizeLimits (W, H, W, H);             // plugin de tamano fijo: la ventana se ajusta a el

    setSize (W, H);
}

void PluginBrowserEditor::applyScale()
{
    const float scales[] = { 0.5f, 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
    const int id = scaleBox.getSelectedId();
    if (id < 1 || id > 6)
        return;

    processor.uiScale = scales[id - 1];
    if (hostedEditor == nullptr)
        return;

    hostedEditor->setScaleFactor (processor.uiScale);

    juce::Component::SafePointer<PluginBrowserEditor> safe (this);
    juce::Timer::callAfterDelay (120, [safe]
    {
        if (safe != nullptr)
            safe->fitToHosted();
    });
}

void PluginBrowserEditor::hostedAboutToChange()
{
    destroyHostedEditor();
}

void PluginBrowserEditor::hostedChanged()
{
    if (processor.getHosted() != nullptr)
    {
        createHostedEditor();
        showPluginView (true);
    }
    else
    {
        showPluginView (false);
    }
}

void PluginBrowserEditor::componentMovedOrResized (juce::Component& c, bool, bool wasResized)
{
    // Si el propio plugin cambia su tamano, la ventana lo sigue
    if (pluginView && wasResized && ! ignoreHostedResize && &c == hostedEditor.get())
        fitToHosted();
}
