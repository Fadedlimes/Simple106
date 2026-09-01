#include "PluginProcessor.h"
#include "PluginEditor.h"

Simple106AudioProcessor::Simple106AudioProcessor()
: AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout Simple106AudioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // DCO 1
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dco1Morph", "DCO1 Morph", 0.0f, 1.0f, 0.33f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dco1PWM", "DCO1 PWM", 0.05f, 0.95f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dco1Level", "DCO1 Level", 0.0f, 1.0f, 0.8f));

    // DCO 2
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dco2Morph", "DCO2 Morph", 0.0f, 1.0f, 0.66f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dco2PWM", "DCO2 PWM", 0.05f, 0.95f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dco2Level", "DCO2 Level", 0.0f, 1.0f, 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("dco2Semi", "DCO2 Semi", -24, 24, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dco2Cents", "DCO2 Cents", -50.0f, 50.0f, 5.0f));

    // Sub & Noise
    params.push_back(std::make_unique<juce::AudioParameterFloat>("subLevel", "Sub Level", 0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("noiseLevel", "Noise Level", 0.0f, 1.0f, 0.0f));

    // Filters
    params.push_back(std::make_unique<juce::AudioParameterFloat>("hpfCutoff", "HPF Cutoff", 10.0f, 1000.0f, 20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lpfCutoff", "LPF Cutoff", 20.0f, 20000.0f, 2500.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lpfRes", "LPF Resonance", 0.0f, 0.98f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("envMod", "Filter Env Mod", -1.0f, 1.0f, 0.5f));

    // Envelopes
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ampAttack", "Amp Attack", 0.001f, 3.0f, 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ampDecay", "Amp Decay", 0.001f, 3.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ampSustain", "Amp Sustain", 0.0f, 1.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("ampRelease", "Amp Release", 0.001f, 5.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("filtAttack", "Filt Attack", 0.001f, 3.0f, 0.05f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filtDecay", "Filt Decay", 0.001f, 3.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filtSustain", "Filt Sustain", 0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filtRelease", "Filt Release", 0.001f, 5.0f, 0.5f));

    // LFO 1
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo1Rate", "LFO 1 Rate", 0.1f, 25.0f, 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo1Shape", "LFO 1 Shape",
                                                                  juce::StringArray{"Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo1ToFilter", "LFO 1 to Filter", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo1ToPitch", "LFO 1 to Pitch", 0.0f, 1.0f, 0.0f));

    // Modulation Targets
    const juce::StringArray modTargets = {
        "None", "LPF Cutoff", "LPF Reso", "HPF Cutoff", "Env Mod",
        "Filt Attack", "Filt Decay", "Filt Sustain", "Filt Release",
        "Amp Attack", "Amp Decay", "Amp Sustain", "Amp Release",
        "DCO1 Morph", "DCO1 PWM", "DCO2 Morph", "DCO2 PWM", "DCO2 Semi", "DCO2 Detune",
        "Sub Level", "Noise Level", "Glide Time", "Master Volume"
    };

    // LFO 2
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo2Rate", "LFO 2 Rate", 0.05f, 25.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo2Shape", "LFO 2 Shape",
                                                                  juce::StringArray{"Sine", "Triangle", "S&H"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo2Amount", "LFO 2 Amount", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo2Target", "LFO 2 Target", modTargets, 0));

    // LFO 3
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo3Rate", "LFO 3 Rate", 0.05f, 25.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo3Shape", "LFO 3 Shape",
                                                                  juce::StringArray{"Sine", "Triangle", "S&H"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo3Amount", "LFO 3 Amount", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo3Target", "LFO 3 Target", modTargets, 0));

    // Voice Variation Matrix
    params.push_back(std::make_unique<juce::AudioParameterChoice>("voiceVarMode", "Var Mode", juce::StringArray{"PANNING", "TUNING"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>("cycleMode", "Voice Cycle", false));

    for (int i = 1; i <= 6; ++i) {
        params.push_back(std::make_unique<juce::AudioParameterFloat>("vPan" + juce::String(i), "Voice " + juce::String(i) + " Pan", -1.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("vTune" + juce::String(i), "Voice " + juce::String(i) + " Tune", -50.0f, 50.0f, 0.0f));
    }

    // Clean Play Mode Choices
    params.push_back(std::make_unique<juce::AudioParameterChoice>("playMode", "Play Mode",
                                                                  juce::StringArray{"Poly", "Mono", "Unison"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("glideTime", "Glide Time", 0.001f, 1.0f, 0.05f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterVolume", "Master Volume", 0.0f, 1.0f, 0.7f));

    // Chord Mode
    params.push_back(std::make_unique<juce::AudioParameterBool>("chordEnable", "Chord Mode", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("chordType", "Chord Type",
                                                                  juce::StringArray{"Major", "Minor", "Maj7", "Min7", "Dom7", "Sus4", "Dim", "Octave", "Power (5th)"}, 0));

    // Arpeggiator
    params.push_back(std::make_unique<juce::AudioParameterBool>("arpEnable", "Arp Mode", false));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("arpMode", "Arp Type",
                                                                  juce::StringArray{"Up", "Down", "Up/Down", "Random", "As Played"}, 0));

    // Sequencer Pages
    params.push_back(std::make_unique<juce::AudioParameterChoice>("seqPages", "Seq Pages",
                                                                  juce::StringArray{"1 PG [16]", "2 PG [32]", "3 PG [48]", "4 PG [64]"}, 3));

    // Theme Selector
    params.push_back(std::make_unique<juce::AudioParameterChoice>("ledTheme", "Theme",
                                                                  juce::StringArray{"Vintage Red", "Neon Cyan", "Acid Green", "Amber Gold", "Solar Yellow", "Ultraviolet", "Ghost White"}, 0));

    // Master FX Suite
    params.push_back(std::make_unique<juce::AudioParameterChoice>("chorusMode", "Chorus",
                                                                  juce::StringArray{"Off", "Chorus I", "Chorus II", "Chorus I+II"}, 1));

    const juce::StringArray syncTimings = {
        "1/32", "1/32t", "1/32d", "1/16", "1/16t", "1/16d",
        "1/8", "1/8t", "1/8d", "1/4", "1/4t", "1/4d",
        "1/2", "1/2t", "1/2d", "1/1", "FREE"
    };

    params.push_back(std::make_unique<juce::AudioParameterChoice>("delaySync", "Delay Sync", syncTimings, 8));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayTime", "Delay Time", 0.05f, 1.5f, 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayFeedback", "Delay Feedback", 0.0f, 0.95f, 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayDamp", "Delay Damp", 0.0f, 0.95f, 0.25f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("delayMix", "Delay Mix", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>("delayPingPong", "Delay Ping-Pong", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbSize", "Reverb Size", 0.0f, 1.0f, 0.65f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbDamp", "Reverb Damp", 0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("reverbMix", "Reverb Mix", 0.0f, 1.0f, 0.0f));

    return { params.begin(), params.end() };
}

juce::File Simple106AudioProcessor::getPresetsDirectory() const {
    auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
    .getChildFile("Fadedlimes")
    .getChildFile("Simple106")
    .getChildFile("Presets");
    if (!dir.exists()) {
        dir.createDirectory();
    }
    return dir;
}

juce::String Simple106AudioProcessor::createPatchXml(const juce::String& patchName) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> root(state.createXml());
    if (root == nullptr) root = std::make_unique<juce::XmlElement>("Simple106Patch");
    root->setAttribute("patchName", patchName);

    auto* seqXml = root->createNewChildElement("Sequencer");
    seqXml->setAttribute("numPages", sequencer.getNumPages());
    seqXml->setAttribute("length", sequencer.getNumSteps());
    for (int i = 0; i < 64; ++i) {
        const auto& step = sequencer.getStep(i);
        auto* stepXml = seqXml->createNewChildElement("Step");
        stepXml->setAttribute("index", i);
        stepXml->setAttribute("note", step.note);
        stepXml->setAttribute("active", step.active ? 1 : 0);
        stepXml->setAttribute("velocity", step.velocity);
    }
    return root->toString();
}

void Simple106AudioProcessor::loadPatchXml(const juce::String& xmlString) {
    auto xml = juce::XmlDocument::parse(xmlString);
    if (xml != nullptr) {
        if (xml->hasTagName(apvts.state.getType()) || xml->hasTagName("Parameters")) {
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
        }
        if (auto* seqXml = xml->getChildByName("Sequencer")) {
            int pages = seqXml->getIntAttribute("numPages", 4);
            sequencer.setNumPages(pages);
            for (auto* stepXml : seqXml->getChildIterator()) {
                if (stepXml->hasTagName("Step")) {
                    int idx = stepXml->getIntAttribute("index", 0);
                    int note = stepXml->getIntAttribute("note", 48);
                    bool active = stepXml->getIntAttribute("active", 0) != 0;
                    float vel = static_cast<float>(stepXml->getDoubleAttribute("velocity", 0.8));
                    auto& step = sequencer.getStep(idx);
                    step.note = note;
                    step.active = active;
                    step.velocity = vel;
                }
            }
        }
    }
}

void Simple106AudioProcessor::loadFactoryPreset(int presetIndex) {
    auto setParam = [this](const char* id, float val) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(val));
    };

    switch (presetIndex) {
        case 0: // 01. Init Poly
            setParam("dco1Morph", 0.33f); setParam("dco1Level", 0.8f);
            setParam("dco2Morph", 0.66f); setParam("dco2Level", 0.5f); setParam("dco2Semi", 0.0f); setParam("dco2Cents", 5.0f);
            setParam("lpfCutoff", 4500.0f); setParam("lpfRes", 0.15f); setParam("envMod", 0.3f);
            setParam("ampAttack", 0.01f); setParam("ampDecay", 0.3f); setParam("ampSustain", 0.8f); setParam("ampRelease", 0.3f);
            setParam("chorusMode", 0.0f); setParam("delayMix", 0.0f); setParam("reverbMix", 0.0f);
            break;

        case 1: // 02. Juno Warm Pad
            setParam("dco1Morph", 0.33f); setParam("dco1Level", 0.7f);
            setParam("dco2Morph", 0.85f); setParam("dco2PWM", 0.65f); setParam("dco2Level", 0.7f); setParam("dco2Cents", 8.0f);
            setParam("subLevel", 0.4f); setParam("lpfCutoff", 1800.0f); setParam("lpfRes", 0.1f); setParam("envMod", 0.4f);
            setParam("ampAttack", 0.45f); setParam("ampDecay", 1.2f); setParam("ampSustain", 0.9f); setParam("ampRelease", 1.5f);
            setParam("filtAttack", 0.6f); setParam("filtDecay", 1.5f); setParam("filtSustain", 0.4f); setParam("filtRelease", 1.8f);
            setParam("chorusMode", 1.0f); setParam("reverbMix", 0.35f); setParam("reverbSize", 0.8f);
            break;

        case 2: // 03. Poly Drift Keys
            setParam("dco1Morph", 0.15f); setParam("dco1Level", 0.8f);
            setParam("dco2Morph", 0.33f); setParam("dco2Level", 0.6f); setParam("dco2Cents", 12.0f);
            setParam("lpfCutoff", 3200.0f); setParam("lpfRes", 0.25f); setParam("envMod", 0.5f);
            setParam("ampAttack", 0.01f); setParam("ampDecay", 0.8f); setParam("ampSustain", 0.4f); setParam("ampRelease", 0.6f);
            setParam("chorusMode", 2.0f); setParam("delayMix", 0.25f); setParam("delaySync", 8.0f); setParam("reverbMix", 0.2f);
            break;

        case 3: // 04. Supersaw Hoover
            setParam("playMode", 2.0f);
            setParam("dco1Morph", 0.33f); setParam("dco1Level", 0.9f);
            setParam("dco2Morph", 0.33f); setParam("dco2Level", 0.9f); setParam("dco2Cents", 25.0f);
            setParam("subLevel", 0.6f); setParam("lpfCutoff", 5000.0f); setParam("lpfRes", 0.3f); setParam("envMod", 0.6f);
            setParam("ampAttack", 0.01f); setParam("ampDecay", 0.4f); setParam("ampSustain", 0.8f); setParam("ampRelease", 0.4f);
            setParam("chorusMode", 3.0f); setParam("reverbMix", 0.25f);
            break;

        case 4: // 05. Acid 106 Lead
            setParam("playMode", 1.0f);
            setParam("glideTime", 0.06f);
            setParam("dco1Morph", 0.33f); setParam("dco1Level", 0.9f); setParam("dco2Level", 0.0f); setParam("subLevel", 0.5f);
            setParam("lpfCutoff", 1200.0f); setParam("lpfRes", 0.75f); setParam("envMod", 0.8f);
            setParam("filtAttack", 0.01f); setParam("filtDecay", 0.25f); setParam("filtSustain", 0.0f); setParam("filtRelease", 0.1f);
            setParam("ampAttack", 0.005f); setParam("ampDecay", 0.3f); setParam("ampSustain", 0.7f); setParam("ampRelease", 0.2f);
            setParam("delayMix", 0.35f); setParam("delaySync", 8.0f); setParam("delayFeedback", 0.55f);
            break;

        case 5: // 06. 80s Arp Dream
            setParam("arpEnable", 1.0f); setParam("arpMode", 2.0f);
            setParam("dco1Morph", 0.5f); setParam("dco1PWM", 0.6f); setParam("dco1Level", 0.8f);
            setParam("dco2Morph", 0.33f); setParam("dco2Level", 0.5f); setParam("dco2Semi", 12.0f);
            setParam("lpfCutoff", 2800.0f); setParam("lpfRes", 0.35f); setParam("envMod", 0.55f);
            setParam("filtDecay", 0.35f); setParam("filtSustain", 0.1f);
            setParam("delayMix", 0.3f); setParam("delaySync", 8.0f); setParam("reverbMix", 0.25f);
            break;

        case 6: // 07. MonoPoly 6-Voice Drift
            setParam("cycleMode", 1.0f);
            setParam("dco1Morph", 0.33f); setParam("dco1Level", 0.8f);
            setParam("dco2Morph", 0.7f); setParam("dco2Level", 0.6f); setParam("dco2Cents", 15.0f);
            setParam("vPan1", -0.8f); setParam("vPan2", 0.8f); setParam("vPan3", -0.4f);
            setParam("vPan4", 0.4f); setParam("vPan5", -0.1f); setParam("vPan6", 0.1f);
            setParam("lpfCutoff", 3400.0f); setParam("lpfRes", 0.2f);
            setParam("delayMix", 0.2f); setParam("reverbMix", 0.2f);
            break;

        case 7: // 08. Space Pluck
            setParam("dco1Morph", 0.0f); setParam("dco1Level", 0.9f);
            setParam("dco2Morph", 0.8f); setParam("dco2Level", 0.4f); setParam("dco2Semi", 7.0f);
            setParam("lpfCutoff", 2200.0f); setParam("lpfRes", 0.4f); setParam("envMod", 0.7f);
            setParam("filtAttack", 0.001f); setParam("filtDecay", 0.2f); setParam("filtSustain", 0.0f);
            setParam("ampAttack", 0.001f); setParam("ampDecay", 0.4f); setParam("ampSustain", 0.0f); setParam("ampRelease", 0.4f);
            setParam("delayMix", 0.4f); setParam("delaySync", 11.0f); setParam("reverbMix", 0.45f); setParam("reverbSize", 0.85f);
            break;
    }
}

void Simple106AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    voiceManager.init(sampleRate);
    lfo1.setSampleRate(sampleRate);
    lfo2.setSampleRate(sampleRate);
    lfo3.setSampleRate(sampleRate);
    chorus.setSampleRate(sampleRate);
    delay.setSampleRate(sampleRate);
    reverb.prepare(sampleRate, samplesPerBlock);
    sequencer.setSampleRate(sampleRate);
    keyboardState.reset();

    activeMidiInputs.clear();
    auto midiDevices = juce::MidiInput::getAvailableDevices();
    for (const auto& dev : midiDevices) {
        if (auto input = juce::MidiInput::openDevice(dev.identifier, this)) {
            input->start();
            activeMidiInputs.push_back(std::move(input));
        }
    }
}

void Simple106AudioProcessor::releaseResources() {
    for (auto& input : activeMidiInputs) {
        input->stop();
    }
    activeMidiInputs.clear();
    keyboardState.reset();
}

void Simple106AudioProcessor::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) {
    keyboardState.processNextMidiEvent(message);
}

bool Simple106AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void Simple106AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Host DAW Transport & BPM Tracking
    double hostBpm = 120.0;
    bool isHostPlaying = false;
    double ppqPos = 0.0;

    if (auto* playHead = getPlayHead()) {
        if (auto pos = playHead->getPosition()) {
            if (pos->getBpm()) hostBpm = *pos->getBpm();
            isHostPlaying = pos->getIsPlaying(); // Direct boolean return in JUCE 8
            if (pos->getPpqPosition()) ppqPos = *pos->getPpqPosition();
        }
    }
    sequencer.setTempo(static_cast<float>(hostBpm));

    // DAW Play/Stop Synchronization
    static bool lastHostPlaying = false;
    if (isHostPlaying != lastHostPlaying) {
        sequencer.isPlaying = isHostPlaying;
        if (isHostPlaying) {
            double sixteenthSteps = ppqPos * 4.0;
            sequencer.currentStep = static_cast<int>(std::fmod(std::floor(sixteenthSteps), static_cast<double>(sequencer.getNumSteps())));
        }
        lastHostPlaying = isHostPlaying;
    }

    // Inject keyboard events into MIDI stream
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    // 1. Fetch Parameters
    float d1Morph = apvts.getRawParameterValue("dco1Morph")->load();
    float d1PWM   = apvts.getRawParameterValue("dco1PWM")->load();
    float d1Level = apvts.getRawParameterValue("dco1Level")->load();

    float d2Morph = apvts.getRawParameterValue("dco2Morph")->load();
    float d2PWM   = apvts.getRawParameterValue("dco2PWM")->load();
    float d2Level = apvts.getRawParameterValue("dco2Level")->load();
    int   d2Semi  = static_cast<int>(apvts.getRawParameterValue("dco2Semi")->load());
    float d2Cents = apvts.getRawParameterValue("dco2Cents")->load();

    float subLvl  = apvts.getRawParameterValue("subLevel")->load();
    float nseLvl  = apvts.getRawParameterValue("noiseLevel")->load();

    float hpfCut  = apvts.getRawParameterValue("hpfCutoff")->load();
    float lpfCut  = apvts.getRawParameterValue("lpfCutoff")->load();
    float lpfRes  = apvts.getRawParameterValue("lpfRes")->load();
    float envMod  = apvts.getRawParameterValue("envMod")->load();

    float aA = apvts.getRawParameterValue("ampAttack")->load();
    float aD = apvts.getRawParameterValue("ampDecay")->load();
    float aS = apvts.getRawParameterValue("ampSustain")->load();
    float aR = apvts.getRawParameterValue("ampRelease")->load();

    float fA = apvts.getRawParameterValue("filtAttack")->load();
    float fD = apvts.getRawParameterValue("filtDecay")->load();
    float fS = apvts.getRawParameterValue("filtSustain")->load();
    float fR = apvts.getRawParameterValue("filtRelease")->load();

    // LFOs
    float lfo1Rate   = apvts.getRawParameterValue("lfo1Rate")->load();
    int   lfo1Shape  = static_cast<int>(apvts.getRawParameterValue("lfo1Shape")->load());
    float lfo1ToFilt = apvts.getRawParameterValue("lfo1ToFilter")->load();
    float lfo1ToPitch= apvts.getRawParameterValue("lfo1ToPitch")->load();

    float lfo2Rate   = apvts.getRawParameterValue("lfo2Rate")->load();
    int   lfo2Shape  = static_cast<int>(apvts.getRawParameterValue("lfo2Shape")->load());
    float lfo2Amount = apvts.getRawParameterValue("lfo2Amount")->load();
    int   lfo2Target = static_cast<int>(apvts.getRawParameterValue("lfo2Target")->load());

    float lfo3Rate   = apvts.getRawParameterValue("lfo3Rate")->load();
    int   lfo3Shape  = static_cast<int>(apvts.getRawParameterValue("lfo3Shape")->load());
    float lfo3Amount = apvts.getRawParameterValue("lfo3Amount")->load();
    int   lfo3Target = static_cast<int>(apvts.getRawParameterValue("lfo3Target")->load());

    // Performance Modes
    bool arpOn     = apvts.getRawParameterValue("arpEnable")->load() > 0.5f;
    int  arpIdx    = static_cast<int>(apvts.getRawParameterValue("arpMode")->load());
    bool chordOn   = apvts.getRawParameterValue("chordEnable")->load() > 0.5f;
    int  chordIdx  = static_cast<int>(apvts.getRawParameterValue("chordType")->load());

    // Delay Sync Timings
    int   delaySyncIdx = static_cast<int>(apvts.getRawParameterValue("delaySync")->load());
    float dFb          = apvts.getRawParameterValue("delayFeedback")->load();
    float dDamp        = apvts.getRawParameterValue("delayDamp")->load();
    float dMix         = apvts.getRawParameterValue("delayMix")->load();
    bool  dPingPong    = apvts.getRawParameterValue("delayPingPong")->load() > 0.5f;

    const float syncMultipliers[] = {
        0.125f, 0.08333f, 0.1875f,
        0.25f,  0.16667f, 0.375f,
        0.5f,   0.33333f, 0.75f,
        1.0f,   0.66667f, 1.5f,
        2.0f,   1.33333f, 3.0f,
        4.0f
    };

    float finalDelayTimeSec = 0.35f;
    if (delaySyncIdx >= 0 && delaySyncIdx < 16) {
        float beatTime = static_cast<float>(60.0 / hostBpm);
        finalDelayTimeSec = beatTime * syncMultipliers[delaySyncIdx];
    } else {
        finalDelayTimeSec = apvts.getRawParameterValue("delayTime")->load();
    }

    int chorusM     = static_cast<int>(apvts.getRawParameterValue("chorusMode")->load());
    float rSize     = apvts.getRawParameterValue("reverbSize")->load();
    float rDamp     = apvts.getRawParameterValue("reverbDamp")->load();
    float rMix      = apvts.getRawParameterValue("reverbMix")->load();

    int playM       = static_cast<int>(apvts.getRawParameterValue("playMode")->load());
    float glide     = apvts.getRawParameterValue("glideTime")->load();
    float masterVol = apvts.getRawParameterValue("masterVolume")->load();

    int  varMode    = static_cast<int>(apvts.getRawParameterValue("voiceVarMode")->load());
    bool cycleOn    = apvts.getRawParameterValue("cycleMode")->load() > 0.5f;

    // 2. Configure Modules
    voiceManager.setPlayMode(playM);
    voiceManager.setCycleMode(cycleOn);
    voiceManager.setChordMode(!arpOn && chordOn, chordIdx);

    sequencer.arpEnabled = arpOn;
    sequencer.setArpMode(arpIdx);

    int numPages = static_cast<int>(apvts.getRawParameterValue("seqPages")->load()) + 1;
    sequencer.setNumPages(numPages);

    lfo1.setFrequency(lfo1Rate);
    lfo1.setWaveform(lfo1Shape);

    lfo2.setFrequency(lfo2Rate);
    lfo2.setWaveform(lfo2Shape == 2 ? 5 : lfo2Shape);

    lfo3.setFrequency(lfo3Rate);
    lfo3.setWaveform(lfo3Shape == 2 ? 5 : lfo3Shape);

    chorus.setMode(chorusM);
    delay.setParameters(finalDelayTimeSec, dFb, dDamp, dMix, dPingPong);
    reverb.setParameters(rSize, rDamp, rMix);

    // Apply Voice Variation
    auto& voices = voiceManager.getVoices();
    for (int i = 0; i < 6; ++i) {
        if (varMode == 0) {
            float pVal = apvts.getRawParameterValue("vPan" + juce::String(i + 1))->load();
            voices[i].setVoicePan(pVal);
            voices[i].setVoiceTuneOffset(0.0f);
        } else {
            float tVal = apvts.getRawParameterValue("vTune" + juce::String(i + 1))->load();
            voices[i].setVoicePan(0.0f);
            voices[i].setVoiceTuneOffset(tVal);
        }
    }

    // 3. Process MIDI
    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        int noteNum = msg.getNoteNumber();

        if (msg.isNoteOn()) {
            if (msg.getVelocity() > 0) {
                if (arpOn) {
                    if (chordOn) {
                        auto chordNotes = voiceManager.getChordIntervals(noteNum);
                        for (int n : chordNotes) {
                            sequencer.addArpNote(n);
                        }
                    } else {
                        sequencer.addArpNote(noteNum);
                    }
                } else {
                    if (sequencer.isRecording) {
                        sequencer.recordNote(noteNum);
                    }
                    voiceManager.handleNoteOn(noteNum, msg.getFloatVelocity(), playM != 0, glide);
                }
            } else {
                if (arpOn) {
                    if (chordOn) {
                        auto chordNotes = voiceManager.getChordIntervals(noteNum);
                        for (int n : chordNotes) {
                            sequencer.removeArpNote(n);
                        }
                    } else {
                        sequencer.removeArpNote(noteNum);
                    }
                } else {
                    voiceManager.handleNoteOff(noteNum);
                }
            }
        } else if (msg.isNoteOff()) {
            if (arpOn) {
                if (chordOn) {
                    auto chordNotes = voiceManager.getChordIntervals(noteNum);
                    for (int n : chordNotes) {
                        sequencer.removeArpNote(n);
                    }
                } else {
                    sequencer.removeArpNote(noteNum);
                }
            } else {
                voiceManager.handleNoteOff(noteNum);
            }
        } else if (msg.isAllNotesOff() || msg.isAllSoundOff()) {
            sequencer.clearArp();
            for (auto& v : voiceManager.getVoices()) v.noteOff();
        }
    }

    // Arpeggiator Clock Processing
    int arpNote = -1;
    float arpVel = 0.85f;
    bool arpOff = false;
    static int lastArpPlayingNote = -1;

    if (arpOn) {
        if (sequencer.advanceArpClock(buffer.getNumSamples(), arpNote, arpVel, arpOff)) {
            if (lastArpPlayingNote >= 0) {
                voiceManager.handleNoteOff(lastArpPlayingNote);
            }
            lastArpPlayingNote = arpNote;
            voiceManager.handleNoteOn(arpNote, arpVel, playM != 0, glide);
        } else if (sequencer.getNumArpHeld() == 0 && lastArpPlayingNote >= 0) {
            voiceManager.handleNoteOff(lastArpPlayingNote);
            lastArpPlayingNote = -1;
        }
    } else {
        if (lastArpPlayingNote >= 0) {
            voiceManager.handleNoteOff(lastArpPlayingNote);
            lastArpPlayingNote = -1;
        }
    }

    // Sequencer Clock processing
    int seqNote = -1;
    float seqVel = 0.8f;
    bool seqOff = false;
    static int lastSeqPlayingNote = -1;

    if (!arpOn && sequencer.advanceStepClock(buffer.getNumSamples(), seqNote, seqVel, seqOff)) {
        if (lastSeqPlayingNote >= 0) {
            voiceManager.handleNoteOff(lastSeqPlayingNote);
        }
        lastSeqPlayingNote = seqNote;
        voiceManager.handleNoteOn(seqNote, seqVel, playM != 0, glide);
    } else if (seqOff) {
        if (lastSeqPlayingNote >= 0) {
            voiceManager.handleNoteOff(lastSeqPlayingNote);
            lastSeqPlayingNote = -1;
        }
        for (auto& v : voiceManager.getVoices()) v.noteOff();
    }

    // 4. Render Audio Block
    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);

    float gainComp = (playM == 2) ? 0.40f : 0.85f;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float lfo1Val = lfo1.process();
        float lfo2Val = lfo2.process() * lfo2Amount;
        float lfo3Val = lfo3.process() * lfo3Amount;

        float modCutoff   = lpfCut;
        float modRes      = lpfRes;
        float modHpf      = hpfCut;
        float modEnvMod   = envMod;
        float modFiltA    = fA, modFiltD = fD, modFiltS = fS, modFiltR = fR;
        float modAmpA     = aA, modAmpD = aD, modAmpS = aS, modAmpR = aR;
        float modD1Morph  = d1Morph, modD1PWM = d1PWM;
        float modD2Morph  = d2Morph, modD2PWM = d2PWM;
        int   modD2Semi   = d2Semi;
        float modD2Cents  = d2Cents;
        float modSub      = subLvl, modNoise = nseLvl;
        float modGlide    = glide;
        float modMaster   = masterVol;

        auto applyModTarget = [&](int target, float modVal) {
            switch (target) {
                case 1:  modCutoff  = std::clamp(modCutoff + modVal * 6000.0f, 20.0f, 20000.0f); break;
                case 2:  modRes     = std::clamp(modRes + modVal * 0.5f, 0.0f, 0.98f); break;
                case 3:  modHpf     = std::clamp(modHpf + modVal * 500.0f, 10.0f, 1000.0f); break;
                case 4:  modEnvMod  = std::clamp(modEnvMod + modVal * 0.5f, -1.0f, 1.0f); break;
                case 5:  modFiltA   = std::clamp(modFiltA + modVal * 1.5f, 0.001f, 3.0f); break;
                case 6:  modFiltD   = std::clamp(modFiltD + modVal * 1.5f, 0.001f, 3.0f); break;
                case 7:  modFiltS   = std::clamp(modFiltS + modVal * 0.5f, 0.0f, 1.0f); break;
                case 8:  modFiltR   = std::clamp(modFiltR + modVal * 2.0f, 0.001f, 5.0f); break;
                case 9:  modAmpA    = std::clamp(modAmpA + modVal * 1.5f, 0.001f, 3.0f); break;
                case 10: modAmpD    = std::clamp(modAmpD + modVal * 1.5f, 0.001f, 3.0f); break;
                case 11: modAmpS    = std::clamp(modAmpS + modVal * 0.5f, 0.0f, 1.0f); break;
                case 12: modAmpR    = std::clamp(modAmpR + modVal * 2.0f, 0.001f, 5.0f); break;
                case 13: modD1Morph = std::clamp(modD1Morph + modVal * 0.5f, 0.0f, 1.0f); break;
                case 14: modD1PWM   = std::clamp(modD1PWM + modVal * 0.4f, 0.05f, 0.95f); break;
                case 15: modD2Morph = std::clamp(modD2Morph + modVal * 0.5f, 0.0f, 1.0f); break;
                case 16: modD2PWM   = std::clamp(modD2PWM + modVal * 0.4f, 0.05f, 0.95f); break;
                case 17: modD2Semi  += static_cast<int>(modVal * 12.0f); break;
                case 18: modD2Cents = std::clamp(modD2Cents + modVal * 30.0f, -50.0f, 50.0f); break;
                case 19: modSub     = std::clamp(modSub + modVal * 0.5f, 0.0f, 1.0f); break;
                case 20: modNoise   = std::clamp(modNoise + modVal * 0.5f, 0.0f, 1.0f); break;
                case 21: modGlide   = std::clamp(modGlide + modVal * 0.4f, 0.001f, 1.0f); break;
                case 22: modMaster  = std::clamp(modMaster + modVal * 0.4f, 0.0f, 1.0f); break;
                default: break;
            }
        };

        applyModTarget(lfo2Target, lfo2Val);
        applyModTarget(lfo3Target, lfo3Val);

        for (auto& voice : voiceManager.getVoices()) {
            voice.ampEnv.setParameters(modAmpA, modAmpD, modAmpS, modAmpR);
            voice.filterEnv.setParameters(modFiltA, modFiltD, modFiltS, modFiltR);
        }

        float sumL = 0.0f, sumR = 0.0f;

        for (auto& voice : voiceManager.getVoices()) {
            float vL = 0.0f, vR = 0.0f;
            voice.process(
                modD1Morph, modD1PWM, d1Level,
                modD2Morph, modD2PWM, d2Level, modD2Semi, modD2Cents,
                modSub, modNoise,
                modHpf, modCutoff, modRes, modEnvMod,
                lfo1Val, lfo1ToFilt, lfo1ToPitch,
                vL, vR
            );
            sumL += vL;
            sumR += vR;
        }

        sumL *= gainComp;
        sumR *= gainComp;

        float chL = 0.0f, chR = 0.0f;
        chorus.process(sumL, sumR, chL, chR);

        float delL = 0.0f, delR = 0.0f;
        delay.process(chL, chR, delL, delR);

        reverb.process(delL, delR);

        outL[sample] = std::tanh(delL * modMaster);
        outR[sample] = std::tanh(delR * modMaster);
    }
}

juce::AudioProcessorEditor* Simple106AudioProcessor::createEditor() {
    return new Simple106AudioProcessorEditor(*this);
}

void Simple106AudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> root(state.createXml());
    if (root == nullptr) root = std::make_unique<juce::XmlElement>("Simple106Patch");

    auto* seqXml = root->createNewChildElement("Sequencer");
    seqXml->setAttribute("numPages", sequencer.getNumPages());
    seqXml->setAttribute("length", sequencer.getNumSteps());
    for (int i = 0; i < 64; ++i) {
        const auto& step = sequencer.getStep(i);
        auto* stepXml = seqXml->createNewChildElement("Step");
        stepXml->setAttribute("index", i);
        stepXml->setAttribute("note", step.note);
        stepXml->setAttribute("active", step.active ? 1 : 0);
        stepXml->setAttribute("velocity", step.velocity);
    }

    auto patchStr = root->toString();
    destData.append(patchStr.toRawUTF8(), patchStr.getNumBytesAsUTF8());
}

void Simple106AudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    juce::String patchStr(static_cast<const char*>(data), static_cast<size_t>(sizeInBytes));
    loadPatchXml(patchStr);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new Simple106AudioProcessor();
}
