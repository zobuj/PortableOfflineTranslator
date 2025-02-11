# RSPCM Software Infastructure

- We need to start to build a software program that will orchastrate the pipeline of data through various ML models that will perform transcription, translation, and speech synthesis. This will also need to support listening to the microcontroller port to decide when it should start to process data. 

- We will need to start with a general implenentation of this data pipeline where we can initally use APIs to query models. Once we get this to work, we can figure out exactly how much memory and storage we will require from these models to determine if we need to do some quantization or add more storage space. This is to be able to have the models on board and queryable offline.

# MCU Software Infastructure

- We are going to need to write software specific to our microconroller to be able to communicate over the I2S protocol to the audio devices, and use I2C to communicate to the LCD screen and the compute module. 

- We are also going to require designing a pipeline to be able to await for a user request to translate then service that request and poll data back from the secondary processing unit. 

- We also have to think about how we are going to be programming the MCU and adding pinouts to be able to actually connect to the micro controller.

# Hardware Logistics

- PCB Design of our microcontroller along with all of the audio subsystem, power subsystem, user interface subsystem, and communication network.

- We need to order all of the components and analyze datasheets to ensure that they have the specifications to work for our application.

- We need to set up a breadboard prototype after we get our software simulations to work. 

- Eventually, we are going to need to design a enclosing for this design to be able to be manageable by an end user.

- We are also going to need to think about the thermal management of our system to make sure to appropriate power delivery to each subsystem.

