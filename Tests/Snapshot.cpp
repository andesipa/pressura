// Renders the plugin window to a PNG so the layout can be checked without a DAW.
// Usage: OPTO2Snapshot out.png [vis] [limit] [ext] [preset=N]
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    Opto2Processor processor;

    for (int i = 2; i < argc; ++i)
    {
        const juce::String arg (argv[i]);
        if (arg.startsWith ("preset="))
            processor.loadPreset (arg.fromFirstOccurrenceOf ("=", false, false).getIntValue());
        else if (auto* p = processor.apvts.getParameter (arg))
            p->setValueNotifyingHost (1.0f);
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);

    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "snapshot.png"));
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat().writeImageToStream (image, stream);
    editor.reset();
    return 0;
}
