/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once //ensure the file is included only once during compilation

#include <JuceHeader.h>
//#include <../MySubmodules/SimpleMultiBandComp/Source/DSP/Fifo.h>
#include <Fifo.h> //First in First out data structure (behaves like a queue or buffer) for passing data between the Audio Thread and the GUI Thread.
/*
What we did in Projucer:
1. Added Header Search Paths: Tells Projecuer "When you write the Visual Studio project file, include this specific folder in the 'Global Search List'
2. The compiler looks at the Search Paths relative to the Project File (.vcxproj) location. They are in Audio_Plugin/Builds/VisualStudio2022.
This is different than writing the full Reative path which says how to get to module from this current file.
3. The .sln (Solution): A container that holds all the different parts of your plugin project.
4. This is the most "important" file. It contains the list of every .cpp file in your project, your optimization settings, and the Header Search Paths.
*/

//==============================================================================
/**
*/
class Audio_PluginAudioProcessor : public juce::AudioProcessor // Base class for audio processors. This is an example of inheritance.
                            #if JucePlugin_Enable_ARA //only include if ARA is enabled
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    Audio_PluginAudioProcessor();
	~Audio_PluginAudioProcessor() override; // override specifier indicates that this function overrides a function  originaly defined in the base/parent class

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    enum class DSP_Option {
        Phase,
		Chorus,
        OverDrive,
		LadderFilter,
        END_OF_LIST,
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvst{ *this, nullptr, "Settings", createParameterLayout() };

	using DSP_Order = std::array<DSP_Option, static_cast<size_t>(DSP_Option::END_OF_LIST)>;
	//using is a way to create an alias (a nickname) for a data type. Here, we are creating an alias called DSP_Order for an array that holds DSP_Option enum values and has a size of 4 (after the static cast).
	SimpleMBComp::Fifo<DSP_Order> dspOrderFifo; //FIFO to pass the DSP order from the GUI thread to the Audio thread. Dictates order of effects applied to audio signal.
	//each instance in the FIFO is a DSP_Order, which is an array of DSP_Option enum values.
private:

    /*
  
	juce::dsp::Phaser<float> phaser; //JUCE's built-in phaser DSP module

	//juce::dsp:: is the namespace where JUCE's digital signal processing classes are defined. This basially says look inside the thing on the left of :: to find the thing on the right.
	//namespace is a way to group related code together to avoid name conflicts in case two things have the same name but are different.
	//<float> is used because Phaser is a template class that can work with different data types, and here we are specifying that we want to use it with floating-point numbers.
	//A template class is a blueprint for creating classes or functions that can work with any data type.
	//phaser (lower case) is the name of the variable we are declaring, which is an instance of the Phaser class.

    
    //At its simplest level, a Template Class is a blueprint for a blueprint. In standard C++, if you wanted a class that 
    //stored integers and another that stored floats, you would have to write two separate classes for esentially the same thing. 
    //With templates, you write the logic once, and the compiler generates the specific versions you need on the fly.
    

	juce::dsp::Chorus<float> chorus; //JUCE's built-in chorus DSP module
	juce::dsp::LadderFilter<float> overdrive, ladderFilter; //JUCE's built-in ladder filter DSP module

    */

	DSP_Order dspOrder; //instance of the enum class defined above

	template <typename DSP_Type> //DSP_Type can be any of the DSP modules Chorus, Phaser, LadderFilter, etc.
    struct DSP_Choice : juce::dsp::ProcessorBase {
		//prepares the selected DSP effect for processing using our settings (spec)
        void prepare(const juce::dsp::ProcessSpec& spec) override {
            dsp.prepare(spec);
        }

		//processes the context (made using the audio buffer) with the selected DSP effect
        void process(const juce::dsp::ProcessContextReplacing<float>& context) override {
            dsp.process(context);
        }

		//resets the state of the selected DSP effect
        void reset() override {
            dsp.reset();
        }

		DSP_Type dsp; //instance of the selected DSP effect
	};
    /*
    The ProcessorWrapper acts as a universal adapter that encases any audio effect in a standard shell, forcing it to follow 
    the ProcessorBase rules of engagement. It provides three specific hooks—prepare for initialization, process for audio math, 
    and reset for clearing memory—which function as direct pass-throughs to the internal effect's logic. This standardization 
    is vital for organizational tools like the ProcessorChain, as it allows the system to treat every distinct effect identically 
    without needing to understand their unique internal designs. The "zero-overhead" nature of C++ templates comes from the fact that 
    they are resolved entirely during compilation rather than while your plugin is running. When you use a template, the compiler 
    performs a process called instantiation, where it generates a specific, concrete version of the code for the exact data type you provided.
    */

	DSP_Choice<juce::dsp::Phaser<float>> phaser; //template param is the DSP module we want to use. We wrap it in the DSP_Choice interface defined above.
    DSP_Choice<juce::dsp::DelayLine<float>> delay;
    DSP_Choice<juce::dsp::Chorus<float>> chorus;
    DSP_Choice<juce::dsp::LadderFilter<float>> overdrive, ladderFilter;

	using DSP_Pointers = std::array<juce::dsp::ProcessorBase*, static_cast<size_t>(DSP_Option::END_OF_LIST)>; //vector of pointers to DSP modules. We will make one instance of DSP_Choice for each DSP module we want to use.
	//The DSP modules we want to use are listed in the enum class DSP_Option defined above.
    //==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Audio_PluginAudioProcessor) //macro to help detect memory leaks and prevent copying of this class
};
