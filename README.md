# Radiance-T 

IOT Device for Non Contact Temperature Measurement and Thermal Image Mapping 

## Inspiration
"**Industry 4.0** fosters what has been called a "smart factory". Within modular structured smart factories, cyber-physical systems monitor physical processes and make decentralized decisions. Industrial Internet of Things (IIoT) systems enable the connectivity of numerous heterogeneous devices and other assets into one system to derive more intelligent actions from data. The application of the IIoT in industrial production systems is known as Industry 4.0".(_[Wikipedia]_ [ABB_D]_)

Industrial automation companies including ABB, Bosch, BMW and Audi envision that in these smart factories of the future people will control the operations and make decisions based on measurement data from the factory's equipment, as well as information regarding the availability of raw materials and the price of energy. This will improve productivity, in more environmentally friendly manner and reduce costs. (_[ABB]_).

Our product, **Radiance-T** will give manufacturers the ability to transition personnel to more value-added activities, providing the foundation to extend and expand product and service offerings. As they look to expand globally, our automated temperature and thermal image monitoring solution will maintain process consistency across locations. The information will be delivered to experts who are far away from the facility itself, for example in applications for the oil and gas industry or for offshore wind power plants. The product can pre-emptively detect and analyze faults and send reports to operation and production planning systems.

## What it does
Our device provides clients with the ability to remotely access data from **Radiance**'s on-board thermographic camera and thermopile sensors to enforce quality control standards of temperature-sensitive goods such as perishables or heat-sensitive chemicals during processing or manufacturing. It enables clients to monitor and perform operations effectively on assembly and manufacturing lines. It provides the visual sight on location where human inspection or visual camera monitoring are difficult.

**Radiance**'s sensors are mounted on a 180-degree swivel platform powered by a remotely controlled servo motor, allowing the client to get temperature readings with a 235-degree field of view. All the logged data is easy accessible to clients via our cloud dashboard.

Clients can also update **Radiance**'s firmware to take advantage of the latest features using the Over The Air Firmware Update capabilities accessible through our cloud application interface/dashboard. 
Increasing automation improves the quality of the entire value chain. The person can use information to program and control operations and make decisions to optimize the quality, safety, efficiency and environmental aspects of production. 

**Application Features** </br>

## How we built it
1. Product design phase with feature and sensor selection.
2. Designing electrical schematics using Altium Designer to enable PCB design.
3. Designing PCB, routing components, satisfying manufacturers Design Rule Checks.
5. Exporting PCB design, submitting it to manufacturer and resolving any last-minute issues.
4. Developing firmware for the PCB, including developing a CLI, Bootloader, and OTAFU functionality using the Atmel Studio IDE and ASF libraries for the SAMD21 MCU.
6. Connecting the hardware to Node-Red using the MQTT broker.
7. Designing UI Interface for **Radiance** using Node-Red IDE with simulation functionality.
8. Perform board bring-up on our manufactured PCBs, and integrate sensors using I2C, Serial and ADC interfaces.

## Challenges we ran into
**Firmware Integration.** Following the checkpoint structure of the course, we developed all the core firmware functionality such as CLI, boot loader, OTAFU and each sensor's interface in independent projects. In the last few weeks of the course when we tried to merge all the software projects to create a singular 2000+ line project, we ran into a lot of conflicts. Examples were using different timers, sensor modules using the same SERCOM pins, or condensing or trying to compress a polling-based while(true) loop using interrupt driven programming. 

**PCB Design.** Our primary challenge in the second stage of our project was narrowing down the functionality and complexity of our board to fit under the $30 / board budget. Significant challenges we faced when tasked with designing the schematics and the board for our PCB, component selection based on size, availability and cost was major challenge. Moreover, complying with PCB manufacture's requirement was most important thing in this phase. 

**PCB Bring-up.**  Testing signal connectivity and voltage levels and making sure power blocks works as we designed them for.

**Sensor Integration** - Interfacing two different I2C sensors on the same I2C line with master processor and calibration of complex temperature sensor on the board.   

**Product Management**- Cost and timeline were major challenges in the development life cycle 

## Accomplishments that we're proud of
+  Build a working prototype for Radiance -T which can be tested with conditions
+  Designed a dashboard and analytics engine which works on cloud computing 
+  IOT practicum and hands on experience on industry standard  product development  
+  Successfully completing a IOT Edge Computing course at Penn

## What we learned
1. Building an IOT device from the ground up. 
2. The building blocks and technical requirements for developing a complete and generic IOT product
3. Circuit Design, PCB design and Firmware development using industry standard software and tools. 
4. Sensor Integration, Cloud connections, Wireless Communication and Power Management techniques
5. Processor, component and material selection and development timeline 
6. IOT design thinking, different phases and challenges in developing a product 

## What's next for Radiance
We are envisioning of building a minimal viable product and testing it on sight with more powerful sensors and analytics engine. We are planning to create a network of Radiance devices and automate the processes for smart factory. While creating Radiance's MVP, we are expanding the application of device in consumer and retail supply chain sector.  Data measurement and predictability will increase the operational reliability, uniformity, cost-effectiveness and safety all while making it easier to control remote locations. 

[Wikipedia]: https://en.wikipedia.org/wiki/Industry_4.0
[ABB]: https://new.abb.com/docs/default-source/technology/a-new-age-of-industrial-production---iotsp.pdf
[ABB_D]: https://new.abb.com/news/detail/11242/digital-twin-a-key-software-component-of-industry-40?fbclid=IwAR1HM_t1Pm8jFNmI8Tb4iujrZ8gK7k-Psb1H3IyOpz_Y-eX-uJeL3S0GhRw


## Course Wrap-up (from Prof. Garcia's email)

Hello Everyone:

Thank you very much for your time and energy in this semester's class. I wanted to send this email as a wrap up of the class, to indicate some tips and next steps that I hope can prove useful. As an additional note, I have attached the Microchip Crypto Primer slides that Dan presented to the class.

### What comes after Altium - Alternatives

In the class we learned about PCB Design using Altium Studio which fortunately Penn has licenses for. There are free alternatives in case you want to continue developing PCBs but do not have access to Altium. They are:

1. [Circuit Maker](https://circuitmaker.com/). This is the online-based free version of Altium. will have most of the utility seen in class.  It is cloud based, which means all the projects will live on the cloud. I have used it and I consider it pretty good.
1. [KiCad](http://kicad-pcb.org/). Open source software for PCB development. Very popular alternative to Altium, and Free! It is also cross platform.
1. [Autodesk Eagle](https://www.autodesk.com/products/eagle/overview). I have not used it since the days it was free. Now it is $15 a month.

### PCB Fabrication
This class used PCB:NG for mixed results. I wanted to share some of my favorite PCB manufacturers with you to keep in mind if you want to do future PCB work.

1. [PCBWAY](https://www.pcbway.com/). Very good PCB fab house, based in China. You can quote the cost on the PCB and assembly online, and their process is very good. You can tell them (for assembly) to buy the components themselves, or send it to them directly from Digikey. I fully recommend them!
1. [Sierra Circuits](https://www.protoexpress.com/). Very good PCB fab house in the States (California). Really good turnaround - I would consider them if you are doing something for work or school - they are not in the price range for hobbyists! Their main advantage lies in that they do a very thorough analisys of your design, and they are very good at "catching" manufacturability issues and letting you know so they can be fixed.

### Interesting Microcontroller Recommendations

There are multiple interesting microcontrollers out there, each with their own advantages. i wanted to share some in case you would like to know more about them. The following options have very inexpensive dev kits you can buy on Amazon or Digikey.

1. ST: I really like ST and use them extensively. They have a supporting software, called CubeMX, which really helps with the phase of HW and FW design (very similar to Atmel Start). Give them a try!  https://www.st.com/content/st_com/en/support/learning/stm32-education.html
1. Rigado: If you need Bluetooth, I heavily recommend a micro from Nordic! Nordic make really good MCUs with built in Bluetooth, but they are very small and sometimes their size alone can drive the cost of the PCB manufacturing (smaller vias and traces equals a more expensive board!). This is sometimes dealt with with the use of modules. Rigado makes modules based on the Nordic chips that take care of complicated electronics needed to run the Nordic chip, and give you a nice module (A la SAMW25). They use a really good (free) development studio called Segger Embedded Studio (free for using the Nordic chips). Give them a try!  https://www.rigado.com/products/wireless-modules/

### Connectors

There exists a ton of connectors, and sometimes you may have seen a connector but have no idea what the name is to find them online! It happens to me a lot.

For this, I recommend the Connector page of Digikey (accesible from the front splash page - on the left, click on "connectors"). Or see this link: https://www.digikey.com/en/resources/connectors/index

You can then go through the connector selection with representative images of the connectors on each category.

### How useful is to know how to make PCBs? And how useful is to do personal projects as an Engineer?

I hope this class gave you the resources you need to bring any electrical project to manufacturing, be it a class project, an academia project, or an industry project.

As a finishing point I wanted to explain how important it is to do personal projects that you can showcase as a portafolio. Projects can be a great way to find new laboral opportunities and to showcase your skills and stand out.

As an example, I wanted to share the work of a friend and colleague. His name is Samson March, and he had been working as a pet project on making his own smart watch. I can attest he worked on it for a really long time. He recently published his project as a Open Source Project, and the project gained so much traction online that he started receiving job opportunities from very interesting companies. You can see the project [here](https://www.theverge.com/circuitbreaker/2019/5/5/18528008/smartwatch-build-your-own-diy-wearable).

I mention this example as a way to motivate you to do your own projects, which I hope this class helped on how to do. Also, on the importance of documentation of said projects! I would recommend, as you do your future school projects or personal projects, to document them AND to take nice pictures! I recommend lightboxes available on Amazon if you want to take nice pictures (Price around 10-50 dollars).


### Keep in Touch

If you want to keep in touch with me, my email is:

- `egarcia@bresslergroup.com`
- `edgarc@seas.upenn.edu`
- LinkedIn: `www.linkedin.com/in/EduardoEEGarcia`

Thank you everyone again for a great semester

Thanks!

-Eduardo
