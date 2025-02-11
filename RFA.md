# Portable Offline Translator

Team Members:
- lorenzo9
- cudia2

# Problem

Traveling is an exciting part of life that can bring joy and new experiences. Trips are the most memorable when everything goes according to plan. However, the language barrier can limit communication with others, causing unnecessary stress on an otherwise enjoyable trip. Although most modern phones provide translation applications, these require a reliable internet connection. In times when the connection is weak or there is no connection at all, translation apps may not be a solution.

# Solution

We want to solve this problem by building a portable translator that you can ideally use anywhere in the world without internet connection. The idea is to have a small device that can be programmed to make translations between two different languages, then is able to listen what the person says, converts the speech to text, translates the text to the target language, then converts the translated text back to speech, and drives a speaker with the target translated speech. We want to design our translator to encompass a few subsystems: Main Processing Subsystem (MCU), Secondary Processing Subsystem (Compute Module), Audio Subsystem, User Interface Subsystem, Communication Subsystem, and Power Management Subsystem. Through this design, someone should be able to turn on the device, set the languages up and start talking into the device, and after a few seconds the translated speech will be played. Then, the device can be programmed the other way to have the other party translate. Ideally, this will facilitate communication between people without a common language and make life easier while traveling.

# Solution Components

## Main Processing Subsystem

Components: 
STM32F407(STM32F407IGH7) (MCU)

The main processing subsystem will manage the workflow for the system, control all I/O, and communicate commands/data to the secondary processing subsystem. When the system powers on a simple interface will be prompted to the user to allow them to select the source and target language to translate. The MCU will support the user inputs through a push button to select the language and will drive the display. Then when the user decides to start translation, through a particular push button, the MCU will change states to start listening on the port for audio data from the microphone. The INMP441 microphone will output a digital signal and communicate over I2S which can be interpreted through our MCU. The MCU will also need to buffer data and need to normalize it to be within the appropriate bit range to be interpreted by the STT model. After preprocessing the data, we will need to set up code to communicate packets of this data over a SPI or I2C protocol to the compute module. We also may need to set up some kind of custom protocol to set the compute module to start listening for a data sequence. Then the compute module will take over and do the translations and conversions to speech and output pulse code modulation data. This data is transmitted again over SPI or I2C to the MCU that is listening. Then the MCU will move to another state to start writing the data to the MAX98357A that will drive the speaker. Then the MCU will move back to a state of user input again to allow the user to translate again. Other than managing the entire workflow for the system, it needs to control the I/O which will include reading inputs from the user on the push buttons and will need to drive an LCD display to show what the user is currently selecting. With enough time, we may also add some status messages onto the LCD display to see what is happening in the system.

We decided to use the STM32F407 for this project because we required high levels of communication between various systems along with the numerous I/O. We also found that it has a LCD parallel interface and JTAG interface. We also have a long reach goal to do some audio manipulation (e.g. filtering, noise reduction) before sending it off to the compute module. We can also expect to support a real time control of the audio and peripheral management.

## Secondary Processing Subsystem

Components: 
Raspberry Pi Compute Module 4 (SC0680/4)

Models:
Speech to Text: Whisper (Tiny Model)
Translation: MarianMT
Text to Speech: Piper

The main purpose of the compute module is to offload high compute tasks, including speech to text transcription, translation, and text to speech conversion. It would be too computationally complex to host all three models on the STM32 while processing I/O data. We specifically chose the RPI Compute Module because it has the computational power to run the AI models, it can interface over SPI or I2C to the MCU, it runs Linux, and it has eMMC Flash to store the models on board. For this subsystem, we are going to have to build an infrastructure around querying the models, reading and sending data to and from the MCU, and a data processing pipeline to move through different stages of translation. It will also need to have some level of state awareness to know what kind of translation is being requested. We hope to design the PCB so that we can simply plug in the compute module onto the PCB through pinouts.

## Audio Subsystem

Components:
INMP441 (I2S Microphone)
MAX98357A (Amplifier)
Dayton Audio CE32A-4 (Speaker)

The audio subsystem encompasses all of the components managing analog signals. Essentially this will include listening to the user voice then delivering that to the MCU. It will also include the amplifier and speaker that are managing converting the digital signals to an analog signal and then speaking out the translated speech. Additionally, the microphone will do analog to digital conversions and some amplification to the signal. To avoid conflict with noise from the speaker with the microphone, we are going to build a very synchronous system where we can only use the microphone or the speaker at a time. 

## User Interface Subsystem

Components:
ST7789 (LCD Display)
Mini Pushbutton Switch

We are going to have a LCD display that can let the user decide what languages to translate between and some push buttons to be able to decide. We are also going to have a push button that will start listening on the microphone, then stop listening so we can ensure that all of the data has been stored. In the case that the translation lasts too long, we may add some feature to automatically stop the input of speech so we make sure not to have too much data to translate. This UI subsystem will essentially make it so that this is a usable product. 

## Communication Subsystem

Protocols:
I2S (Audio I/O)
SPI/I2C (MCU-CM Communication)

We are going to support communicating with I2S between the MCU and both of the audio devices, and we are going to have SPI/I2C communication to the compute module. This may also require some circuit level design to add to the I2C or I2S lines (pull-up resistors).


## Power Management Subsystem

Voltage Levels:
Power Rails: 
5V (CM)
3.3V (MCU, LCD, Audio)
LM317DCYR (Adjustable LDO) X2
18650 Battery Holder 
Samsung 25R 18650 2500mAh 20A Battery

This portable power management system will use a Samsung 25R 18650 2500mAh 20A rechargeable Li-Ion battery to supply stable voltage to two power rails. We will use the 5V power rail for the Raspberry Pi Compute Module (CM) and 3.3V power rail for the MCU, LCD, and audio subsystem. This system includes two LM317DCYR adjustable LDO regulators. One will be used to step down the battery voltage to 5V and the another to step down the voltage to 3.3V. Lastly, the 18650 battery holder securely holds the battery and enables easy replacement.

# Criterion For Success

The basic goals that we hope to achieve with this design is a system where we can press a button to listen to our voice then stop listening, process this data to the compute module, translate the speech, and drive a speaker to hear the translated speech and this speech needs to sound cohesive and be understood. At the simplest level, we hope to translate between English to another language. At the minimum, if this works we can expect to be able to add additional functionality to support more than two languages. We also believe that our system is going to be very synchronous, as if one thread is being used to move the data through all the pipeline. A more asynchronous system would be able to listen and start speaking right as we are talking, but there are more caveats to that system that may be too complicated to get done within one semester. We are able to test this at a high level as we are able to hear what the translated speech is, and we can dive deeper by having stages within our translation pipeline where we can dump the status of the translation to see what exactly we received as input and what we got as output. We also hope to be able to have this be a simple portable design with an encasing to make it very easy to use especially through the UI. 


















MARKDOWN FORMATv

# Portable Offline Translator

Team Members:
- lorenzo9
- cudia2

# Problem

Traveling is an exciting part of life that can bring joy and new experiences. Trips are the most memorable when everything goes according to plan. However, the language barrier can limit communication with others, causing unnecessary stress on an otherwise enjoyable trip. Although most modern phones provide translation applications, these require a reliable internet connection. In times when the connection is weak or there is no connection at all, translation apps may not be a solution.

# Solution

We want to solve this problem by building a portable translator that you can ideally use anywhere in the world without internet connection. The idea is to have a small device that can be programmed to make translations between two different languages, then is able to listen what the person says, converts the speech to text, translates the text to the target language, then converts the translated text back to speech, and drives a speaker with the target translated speech. We want to design our translator to encompass a few subsystems: Main Processing Subsystem (MCU), Secondary Processing Subsystem (Compute Module), Audio Subsystem, User Interface Subsystem, Communication Subsystem, and Power Management Subsystem. Through this design, someone should be able to turn on the device, set the languages up and start talking into the device, and after a few seconds the translated speech will be played. Then, the device can be programmed the other way to have the other party translate. Ideally, this will facilitate communication between people without a common language and make life easier while traveling.

# Solution Components

## Main Processing Subsystem

Components: 
[STM32F407](https://www.st.com/en/microcontrollers-microprocessors/stm32f407-417.html)([STM32F407IGH7](https://www.st.com/resource/en/datasheet/stm32f405zg.pdf)) (MCU)

The main processing subsystem will manage the workflow for the system, control all I/O, and communicate commands/data to the secondary processing subsystem. When the system powers on a simple interface will be prompted to the user to allow them to select the source and target language to translate. The MCU will support the user inputs through a push button to select the language and will drive the display. Then when the user decides to start translation, through a particular push button, the MCU will change states to start listening on the port for audio data from the microphone. The INMP441 microphone will output a digital signal and communicate over I2S which can be interpreted through our MCU. The MCU will also need to buffer data and need to normalize it to be within the appropriate bit range to be interpreted by the STT model. After preprocessing the data, we will need to set up code to communicate packets of this data over a SPI or I2C protocol to the compute module. We also may need to set up some kind of custom protocol to set the compute module to start listening for a data sequence. Then the compute module will take over and do the translations and conversions to speech and output pulse code modulation data. This data is transmitted again over SPI or I2C to the MCU that is listening. Then the MCU will move to another state to start writing the data to the MAX98357A that will drive the speaker. Then the MCU will move back to a state of user input again to allow the user to translate again. Other than managing the entire workflow for the system, it needs to control the I/O which will include reading inputs from the user on the push buttons and will need to drive an LCD display to show what the user is currently selecting. With enough time, we may also add some status messages onto the LCD display to see what is happening in the system.

We decided to use the STM32F407 for this project because we required high levels of communication between various systems along with the numerous I/O. We also found that it has a LCD parallel interface and JTAG interface. We also have a long reach goal to do some audio manipulation (e.g. filtering, noise reduction) before sending it off to the compute module. We can also expect to support a real time control of the audio and peripheral management.

## Secondary Processing Subsystem

Components: 
- [Raspberry Pi Compute Module 4](https://datasheets.raspberrypi.com/cm4/cm4-datasheet.pdf) (SC0680/4)

Models:
- Speech to Text: [Whisper](https://github.com/openai/whisper) (Tiny Model)
- Translation: [MarianMT](https://marian-nmt.github.io/)
- Text to Speech: [Piper](https://github.com/rhasspy/piper)

The main purpose of the compute module is to offload high compute tasks, including speech to text transcription, translation, and text to speech conversion. It would be too computationally complex to host all three models on the STM32 while processing I/O data. We specifically chose the RPI Compute Module because it has the computational power to run the AI models, it can interface over SPI or I2C to the MCU, it runs Linux, and it has eMMC Flash to store the models on board. For this subsystem, we are going to have to build an infrastructure around querying the models, reading and sending data to and from the MCU, and a data processing pipeline to move through different stages of translation. It will also need to have some level of state awareness to know what kind of translation is being requested. We hope to design the PCB so that we can simply plug in the compute module onto the PCB through pinouts.

## Audio Subsystem

Components:
- [INMP441](https://invensense.tdk.com/wp-content/uploads/2015/02/INMP441.pdf) (I2S Microphone)
- [MAX98357A](https://www.analog.com/media/en/technical-documentation/data-sheets/max98357a-max98357b.pdf) (Amplifier)
- [Dayton Audio CE32A-4](https://www.daytonaudio.com/images/resources/285-103-dayton-audio-ce32a-4-spec-sheet.pdf) (Speaker)

The audio subsystem encompasses all of the components managing analog signals. Essentially this will include listening to the user voice then delivering that to the MCU. It will also include the amplifier and speaker that are managing converting the digital signals to an analog signal and then speaking out the translated speech. Additionally, the microphone will do analog to digital conversions and some amplification to the signal. To avoid conflict with noise from the speaker with the microphone, we are going to build a very synchronous system where we can only use the microphone or the speaker at a time. 

## User Interface Subsystem

Components:
- [ST7789](https://cdn-learn.adafruit.com/downloads/pdf/2-0-inch-320-x-240-color-ips-tft-display.pdf) (LCD Display)
- Mini Pushbutton Switch

We are going to have a LCD display that can let the user decide what languages to translate between and some push buttons to be able to decide. We are also going to have a push button that will start listening on the microphone, then stop listening so we can ensure that all of the data has been stored. In the case that the translation lasts too long, we may add some feature to automatically stop the input of speech so we make sure not to have too much data to translate. This UI subsystem will essentially make it so that this is a usable product. 

## Communication Subsystem

Protocols:
- I2S (Audio I/O)
- SPI/I2C (MCU-CM Communication)

We are going to support communicating with I2S between the MCU and both of the audio devices, and we are going to have SPI/I2C communication to the compute module. This may also require some circuit level design to add to the I2C or I2S lines (pull-up resistors).


## Power Management Subsystem

Voltage Levels:
- Power Rails:
  - 5V (CM)
  - 3.3V (MCU, LCD, Audio)

- [LM317DCYR](https://www.digikey.com/en/products/detail/texas-instruments/LM317DCYR/443739?gclsrc=aw.ds&&utm_adgroup=Integrated%20Circuits&utm_source=google&utm_medium=cpc&utm_campaign=Dynamic%20Search_EN_Product&utm_term=&utm_content=Integrated%20Circuits&utm_id=go_cmp-120565755_adg-9159612915_ad-665604606680_dsa-171171979035_dev-c_ext-_prd-_sig-CjwKCAiAtYy9BhBcEiwANWQQL3rwnMSMyK-ZG3XMXp5nru6UD3FAfss_oPQyDGeG0f-Hh2RLp4BssBoCgKIQAvD_BwE&gad_source=1&gclid=CjwKCAiAtYy9BhBcEiwANWQQL3rwnMSMyK-ZG3XMXp5nru6UD3FAfss_oPQyDGeG0f-Hh2RLp4BssBoCgKIQAvD_BwE&gclsrc=aw.ds) (Adjustable LDO) X2
- [18650 Battery Holder](https://www.digikey.com/en/products/detail/keystone-electronics/1048P/4499389?gclsrc=aw.ds&&utm_adgroup=&utm_source=google&utm_medium=cpc&utm_campaign=PMax%20Shopping_Product_Low%20ROAS%20Categories&utm_term=&utm_content=&utm_id=go_cmp-20243063506_adg-_ad-__dev-c_ext-_prd-4499389_sig-CjwKCAiAtYy9BhBcEiwANWQQL67T2WMFu7kbd5dYcLA8KUBEH5cQLd09vsCRMQqPdleIVlpNC45jqhoCQ38QAvD_BwE&gad_source=1&gclid=CjwKCAiAtYy9BhBcEiwANWQQL67T2WMFu7kbd5dYcLA8KUBEH5cQLd09vsCRMQqPdleIVlpNC45jqhoCQ38QAvD_BwE&gclsrc=aw.ds) 
- [Samsung 25R 18650 2500mAh 20A Battery](https://www.18650batterystore.com/products/samsung-25r-18650?utm_campaign=859501437&utm_source=g_c&utm_medium=cpc&utm_content=201043132925&utm_term=_&adgroupid=43081474946&gad_source=1&gclid=CjwKCAiAtYy9BhBcEiwANWQQL9KXMQZ0cgicw8SuV3VKk3KPHvVdYIrlZydGXjFnZ7StpWRljqYGjRoCGikQAvD_BwE)



This portable power management system will use a Samsung 25R 18650 2500mAh 20A rechargeable Li-Ion battery to supply stable voltage to two power rails. We will use the 5V power rail for the Raspberry Pi Compute Module (CM) and 3.3V power rail for the MCU, LCD, and audio subsystem. This system includes two LM317DCYR adjustable LDO regulators. One will be used to step down the battery voltage to 5V and the another to step down the voltage to 3.3V. Lastly, the 18650 battery holder securely holds the battery and enables easy replacement.

# Criterion For Success

The basic goals that we hope to achieve with this design is a system where we can press a button to listen to our voice then stop listening, process this data to the compute module, translate the speech, and drive a speaker to hear the translated speech and this speech needs to sound cohesive and be understood. At the simplest level, we hope to translate between English to another language. At the minimum, if this works we can expect to be able to add additional functionality to support more than two languages. We also believe that our system is going to be very synchronous, as if one thread is being used to move the data through all the pipeline. A more asynchronous system would be able to listen and start speaking right as we are talking, but there are more caveats to that system that may be too complicated to get done within one semester. We are able to test this at a high level as we are able to hear what the translated speech is, and we can dive deeper by having stages within our translation pipeline where we can dump the status of the translation to see what exactly we received as input and what we got as output. We also hope to be able to have this be a simple portable design with an encasing to make it very easy to use especially through the UI. 


