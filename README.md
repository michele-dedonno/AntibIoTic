# Antib*IoT*ic: the Fog-enhanced Distributed Security System to Protect the (Legacy) Internet of Things

![architecture](./antibiotic_2.0.png?raw=true "AntibIoTic 2.0")

This is the official repository for **Antib*IoT*ic**, a distributed security system that relies on Fog computing to protect legacy IoT endpoints. 
The system is based on scientific papers [[SEDA'16]](https://link.springer.com/chapter/10.1007/978-3-319-70578-1_7) [[ICFEC'19]](https://ieeexplore.ieee.org/abstract/document/8733144)[[EuroS&PW'19]](https://ieeexplore.ieee.org/abstract/document/8802381), [[JCS'21]](http://www2.compute.dtu.dk/~xefa/files/journal/2021-jcs-antibiotic.pdf), Ph.D., M.Sc., and B.Sc. theses.

A video demo showing some of the core features implemented in this repository is available [here](https://www.youtube.com/watch?v=xiIKLREo3vY).

We are constantly working to implement and improve **Antib*IoT*ic**. It is a lot of work, but we are doing our best! :)

We mainly work offline and we update this repository when we have a stable version of the code. 
Right now, the code included in this repository implements only a subset of the functionalities of **Antib*IoT*ic**. In particular, it implements the edge of the Antib*IoT*ic architecture characterized by the interaction of a Fog node, running the Antib*IoT*ic gateway, and 3 IoT devices, running the Antib*IoT*ic agent.

For the full design and list of features please refer to the latest published research paper [[JCS'21]](http://www2.compute.dtu.dk/~xefa/files/journal/2021-jcs-antibiotic.pdf).

**If Antib*IoT*ic was useful for your research, you are kindly asked to cite the latest published Antib*IoT*ic paper [[JCS'21]](https://content.iospress.com/articles/journal-of-computer-security/jcs210027).**

> Michele De Donno and the **Antib*IoT*ic** team.

## Directory Structure
- [./tools](./tools): directory containg tools to help development and testing of Antib*IoT*ic. 
- [./Agent](./Agent): directory containing everything related to the Antib*IoT*ic agent.
- [./Server](./Server): directory containing everything related to the Antib*IoT*ic gateway.

## Deployment and Execution
To run the code in this repository, please refer to the detailed istructions provided in each folder.

As an overview, the following steps should be followed:
1. configure the Fog node following the istructions provided in [./Server](./Server);
2. configure each IoT device following the istructions provided in [./Agent](./Agent);
3. generate and copy all required files in [./Server/.upload/](./Server/.upload) folder. The list of required files can be found in [./Server/README.md](./Server/README.md);
4. compile and run the server. By default, it will automatically upload the agent on each IoT device.

## Main Contributors
- [Michele De Donno](https://www.linkedin.com/in/michele-dedonno/) (Main contributor and initiatior, Ph.D. thesis)
- Juan Manuel Donaire Felipe (M.Sc. thesis)
- Mathies Svarrer-Lanthén (B.Sc. thesis)
- Léo Gigandon (M.Sc. level project)

Many other people were involved and contributed to the design of Antib*IoT*ic. 
Here, we mentioned only contributors to the code available in this repository.

## Disclaimer and License
Although the aim is to fully build a security system for the Internet of Things, this project is currently intended for academic and research purposes, thus, you should use it with caution and at your own risk. 

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

This program is licensed under the GNU General Public License v3.0.

## Copyright
Copyright © 2017-2021 Michele De Donno, Technical University of Denmark.
All rights reserved.
