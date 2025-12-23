/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Audio_PluginAudioProcessor::Audio_PluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

Audio_PluginAudioProcessor::~Audio_PluginAudioProcessor()
{
}

//==============================================================================
const juce::String Audio_PluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Audio_PluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Audio_PluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Audio_PluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Audio_PluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Audio_PluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Audio_PluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Audio_PluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Audio_PluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void Audio_PluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Audio_PluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void Audio_PluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Audio_PluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Audio_PluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto newDSPOrder = DSP_Order();

    while (dspOrderFifo.pull(newDSPOrder))
    {
        /*
        .pull() checks the FIFO for new data. If there is data available it returns true.
        The function takes arguments by refrence. Since we pass newDSPOrder by refrence,
		the pull function will set newDSPOrder to the most recent DSP_Order from the FIFO.
        */
    }

    if (newDSPOrder != DSP_Order()) {
		dspOrder = newDSPOrder; //update the global,private dspOrder variable to the latest order from the FIFO.
    }

    
	//convert each enumed dsp in dspOrder into a pointers to their corresponding DSP_Choice objects.
	//store these pointers in an instance of the DSP_Pointers alias type (array) we defined in the header file.
    DSP_Pointers dspPointers;

    for (size_t i = 0; i < dspPointers.size(); ++i)
    {
        switch (dspOrder[i]) //each element in dspOrder is an enum from DSP_Options
        {
            case DSP_Option::Phase:
                dspPointers[i] = &phaser;
                break;
            case DSP_Option::Chorus:
                dspPointers[i] = &chorus;
                break;
            case DSP_Option::OverDrive:
                dspPointers[i] = &overdrive;
                break;
            case DSP_Option::LadderFilter:
                dspPointers[i] = &ladderFilter;
                break;
			case DSP_Option::END_OF_LIST:
                jassertfalse;
                break;
        }
    }

	//process the audio buffer through each DSP effect in the order specified by dspOrder.
    auto block = juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(block);

    for (size_t i = 0; i < dspPointers.size(); ++i)
    {
        if (dspPointers[i] != nullptr)
        {
			dspPointers[i]->process(context);//runs the process function of the DSP_Choice object pointed to by dspPointers[i]. Arrow instead of dot because dspPointers[i] is a pointer.
        }
    }

}

//==============================================================================
bool Audio_PluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Audio_PluginAudioProcessor::createEditor()
{
    return new Audio_PluginAudioProcessorEditor (*this);
}

//==============================================================================
void Audio_PluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Audio_PluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Audio_PluginAudioProcessor();
}
