# MVMNT - Bezier Curve Smooth Random Control Voltage
Breathe Life Into Your Eurorack Modular Patches!

![MVMNT Bezier Curves Smooth Random Control Voltage generator](https://dl.modulove.de/module/mvmnt/Modulove_MVMNT_Productshot_Front.jpg)

MVMNT is an Eurorack module designed to enrich your patch with smooth random and dynamic modulation.
MVMNT offers both precision and versatility, giving you fine control of shape and intensity of the smooth random control voltage.

This is our adaption of the original [HAGIWO 58th original eurorack modular synth DIY project the Bezier curve random CV generator](https://note.com/solder_state/n/n39aacefd73a3) for the [HAGIWO #12 SYNC LFO](https://note.com/solder_state/n/n4c600f2431c3).

# History

All began with the publication of the [original design by Hagiwo](https://note.com/solder_state/n/n39aacefd73a3) in October 2022, this hardware is based on Michael Zülch's ([Cats-Synth](https://github.com/mzuelch/CATs-Eurosynth)) LFO, with many contributions from the eurorack modular community.
It is inspired by the CV section of Mutable Instruments Marbles and we aimed to make it SYNTH DIY friendly and accessible to beginners as well as enthusiasts.
Since more and more Firmware versions emerged for the hardware we picked this as a first favorite and share it with you.

![MVMNT Flip panel ](https://dl.modulove.de/module/mvmnt/Modulove_MVMNT_Productshot_Both.jpg)

# Features:

- Precise control over the shape and rate of smooth random modulation
- Adjustable intensity for fine-tuning randomness
- Dual-Panel Design by [bkrsmdesign](https://www.instagram.com/bkrsmdesign/ "Sasha Kruse"):
  It comes with a flip panel, featuring the original SYNC MOD LFO on the backside.
  This dual-use design allows you to switch between the two modules, providing flexibility and added value.
  
- Three Options in One: Choose the path that suits you best with three options:
- complete built module for immediate gratification,
- complete DIY kit,
- or panel/PCB kit with pre-placed SMD components for the perfect entry point into SYNTH DIY.
  
- Beginner-Friendly: We hope to make it accessible to beginners by keeping assembly straightforward.
  With very few parts to assemble, MVMNT welcomes newcomers to the diy world of modular synths.
  Our build video and firmware uploader ensure a smooth setup for all skill levels.
  
- Smooth Random Control Voltage (CV): MVMNT introduces the power of smooth random CV, enhancing your modular with dynamic and unpredictable modulation possibilities.
- Say goodbye to static soundscapes and welcome the beauty of randomness.
  
- Track & Hold with using the TRIG Input.
  
- Additional inverted and Bipolar Outputs.
  
- Additional Firmware Options from [Adam Wonak](https://github.com/awonak/). The MVMNT hardware offers a wide range of [additional firmware options](https://awonak.github.io/HagiwoModulove/synclfo/) to try. Very easy to flash from your browser-

# Firmware

[Flash the firmware](https://dl.modulove.de/module/mvmnt/) to your module directly from your web browser! No need to compile or upload from the Arduino IDE!


 - [Bezier Curve Smooth Random Control](https://dl.modulove.de/module/mvmmnt/) (Default): The core feature that unlocks smooth, enigmatic control voltage for dynamic modulation.
 - [SYNC MOD LFO original Firmware](https://dl.modulove.de/module/mvmnt/): Complex CV generator with self moduloation
 - [Perlin Noise Firmware](https://awonak.github.io/HagiwoModulove/synclfo/#perlin-noise): This firmware produces smooth random voltages to chaotic noise using the Perlin noise algorithm.
 - [ADSR - envelope generator firmware](https://awonak.github.io/HagiwoModulove/synclfo/#adsr): Craft intricate envelope shapes to shape your sound with precision.
 - [Baby4 - 4 step cv sequencer firmware](https://awonak.github.io/HagiwoModulove/synclfo/#baby4): Dive into the world of CV sequencing.
 - [Multi-mode Envelope Generator firmware](https://awonak.github.io/HagiwoModulove/synclfo/#multimodeenv): 
 - [Generative Polyrhythms](https://awonak.github.io/HagiwoModulove/synclfo/#polyrhythm) based on 16 step counter knobs.
 - [Generative CV Sequencer](https://awonak.github.io/HagiwoModulove/synclfo/#generativesequencer) (Inspired by the Deja Vu mode of the Mutable Instruments Marbles)


# Knob Functions - Bezier curve smooth Random Voltage:

1. ELEVATE is the Level Knob and sets the voltage range and controls output amplitude. 0 to 10V range.

2. STRETCH σ (Standard Deviation) Knob controls the width of frequency fluctuation. At 0, voltage changes at a constant frequency. Increasing the value intensifies the fluctuation, introducing an element of unpredictability- Low = slow changes, High = fast changes

3. SMOOTH Curve Knob Modifys the curvature of the voltage change. At 0, the change is linear, while higher values introduce greater curvature, conforming to Smooth Bezier curve transitions. 0% = Linear transitions, 100% = Smooth Bezier transitions

4. FLUCTUATE Frequency Knob: Adjust the randomness of frequency changes, allowing you to control the speed of modulation. 0% = steady timing, 100% = highly random timing
   
5. Track & Hold: Holds the last voltage when HIGH, resumes normal changes when LOW


# Hardware

 - Arduino Nano v3 (USB-C)
 - 4 Potentiometer knobs
 - TRIG Input (0-5V)
 - Output Voltage 0/+10V
 - Inverted Voltage +/-10V
 - Bipolar Voltage +/-10V
 - Skiff Friendly (24mm depth)

# MVMNT experience

This module quickly became a essential companion to breathe life into our modular syth patches. MVMNT offers the automation and randomness we need to focus more deeply on composition and exploration.



# DIY Info / Links:

Building the Module is simple and suited for beginners. All Kits come with SMD parts already soldered.

Assembly Instructions (Quickstart) here: [MVMNT / SYNC LFO QuickStart](https://modulove.io/mvmnt/ "MVMNT BUILD DOCS").

[MVMNT / SYNC LFO iBOM](https://htmlpreview.github.io/?https://github.com/modulove/MVMNT/blob/main/BOM/ibom.html "MVMNT BOM").

[Buildvideo](https://www.youtube.com/watch?v=wJnPb67zS5Q)

[BenjieGenrativePatch](https://www.youtube.com/watch?v=Tm7N2I8bMkU)

To update / load firmware on to your Arduino Nano you can use the [MVMNT Firmware uploader in Chrome Browser here:](https://dl.modulove.de/module/mvmnt/ "MVMNT Firmware uploader").

Modulargrid: [MVMNT on modulargrid.net](https://www.modulargrid.net/e/modulove-mvmnt "MVMNT on modulargrid.net").

More Info:

- [HAGIWO Bezier curve smooth random voltage generator Article](https://note.com/solder_state/n/n39aacefd73a3 "HAGIWO Bezier curve smooth random voltage module on note.com").

- [HAGIWO SYNC LFO Article](https://note.com/solder_state/n/n4c600f2431c3 "HAGIWO SYNC MOD LFO article").

- [HAGIWO Bezier curve smooth random voltage Video](https://www.youtube.com/watch?v=yJ8VUiZf6Yg "HAGIWO Bezier curve smooth random voltage generator on Youtube").

- [HAGIWO Clock SYNC LFO Video](https://www.youtube.com/watch?v=m64jyVz2w3M "HAGIWO Clock SYNC LFO Youtube Video").


Thanks for all the inspiration and work has been done go out especially to [HAGIWO](https://www.youtube.com/@HAGIWO "HAGIWO Youtube Channel"), [Michael Zülch](https://github.com/mzuelch "Michael Zülch Github Page") and all other people envolved in this corner!
