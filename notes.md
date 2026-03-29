# Soundchip
----
## Spec
* 4 Voices, oscillator agnostic
* ADSR support, LFO support (Volume, Pulse-width modulation, Pitch)
* use getNote() to get frequency of notes
* All timing is derived from sample rate
* dt = frequency / sample rate
----
## How does it work? 
* Oscillators
	- Sine - a basic sine wave
	- Square -
		+ Compute t (phase) & dt (phase change per sample)
		+ Get pulse width and clamp it 0.02f <= pw <= 0.98f.
		+ Correct once when phase changes from 1.0f -> 0.0f
		+ Update phase
		+ Correct again once phase changes from negative to positive
		+ A square wave is simply a wave that changes from 1.0f -> -1.0f, however some correction is needed when the phase is changing because very sharp corners lead to some ugly harmonics so we need to round off the corners with PolyBLEP.
	- Triangle wave -
		+ A triangle wave is a wave that is folded into a zig-zag pattern, however there is still an issue of ugly harmonics with sharp corners. As a shortcut, we just call into the square wave oscillator and then use a leaky integrator as integrating normally will just pull the sample into positive infinity, multiplying by 0.999f simply pulls it back down. We need to call the square sample as we already get an anti-aliased signal for free from the square wave.
		+ Compute the square sample
		+ Get the integrator state and add the square sample
		+ Multiply the integrator state and the amplitude scale to get the sample.
	- Noise -
		+ Simply update phase with dt (phase change per sample)
		+ Once phase >= 1.0f, wrap phase around, change noise_value with lfsr()
* Voices
	- Voices contain oscillators that are sampled once voices are sampled
	- Additionally, voices have an ADSR envelope and an LFO.
		+ ADSR -
			* An ADSR basically controls the volume of a sample as it plays.
			* ADSR Stands for Attack, Decay, Sustain, Release
			* Attack - Attack is for when the sample has just started playing and the volume is rising to its peak. (Measured in Seconds)
			* Decay - Decay is for when the sample has reached its peak and is starting to decay into the sustain. (Measured in Seconds)
			* Sustain - Sustain is for when the sample is sustaining a volume. (Volume level)
			* Release - Release is for when the sample is being let go and volume is decreasing to 0. (Measured in Seconds)
		+ LFO -
			* A slow oscillator that can be used to control different variables of a sound.
			* Currently, the sound chip supports Pulse-Width Modulation, Pitch, and Volume as targets for the LFO to control.
	- When voices are sampled, they first get the LFO's current value and apply it depending on the target. Next, they sample from the oscillator. Afterwards, they apply the envelope for the volume and then return the sample.
	- A voice attacking and releasing is controlled by the voice_note_on() & voice_note_off() functions respectively.
- SoundChip
	+ The soundchip contains 4 voices that can be freely configured. However, the default configuration is Sine, Square, Triangle,and Noise.
	+ Once a soundchip is sampled, it samples all of its enabled voices at once. It adds all of the sample values into a cumulative sample and then clamps it -1.0f <= x <= 1.0f. 
	+ There is a generic callback soundchip_generate() that fills a stream with samples that can be used to generate samples for SDL2 and other libraries.
