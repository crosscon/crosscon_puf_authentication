## PUF-based Authentication Proof of Concept Implementation

This repository hosts the first proof of concept implementation of the PUF-based authentication service developed as part of the [CROSSCON](https://crosscon.eu/) project. This further contributed to the Deliverable D3.2: CROSSCON Open Security Stack – Initial Version.

## Overview

CROSSCON is dedicated to developing a unified software stack tailored for resource-constrained IoT devices. In alignment with this objective, we are introducing a series of innovative, trusted services aimed at bolstering the security of IoT devices and their services. This repository hosts the implementations of our PUF-based authentication services, as detailed in Deliverable D3.1.

This repository features implementations of three distinct solutions. While the current versions are initial and subject to future modifications throughout the project's duration, they represent a solid foundation for development and testing.

Each folder in the repository corresponds to a project and includes one authentication service. For each service, we provide example datasets that contain various PUF challenges along with corresponding enrollment data. This setup facilitates understanding and interaction with the authentication mechanisms we have developed.

The proof of concept implementation was conducted and tested on the LPC55S69-EVK development board from NXP.

## Running the examples

Each folder contains an independent project developed using NXP's MCUXpresso IDE. The simplest way to run these projects is to import them into MCUXpresso and utilize the accompanying toolchain to flash the software onto the device. Alternatively, the projects can be imported into Visual Studio Code, where the MCUXpresso SDK extension can be used to facilitate development and deployment.
Each implementation includes both the authentication and verification processes directly on the device. Verification often involves interactions to retrieve information from public storage; for simplicity, we provide exemplary data to demonstrate the functionality. Specifically, this includes Path data 1-8, for PAVOC and PAWOS as well as an ID. These Path data correspond to the enrollment of a chain or a respective one-time signature pair, along with a path to an exemplary Merkle tree. This Merkle tree contains more than one million enrolled challenge and response pairs. The ID is the root of the Merkle Tree containing all enrolled samples of the Prover.
When you run the example, an authentication message is first generated. This message is then passed to the verification algorithm to confirm its authenticity.

## Acknowledgments

The work presented in this repository is part of the [CROSSCON project](https://crosscon.eu/) that received funding from the European Union’s Horizon Europe research and innovation programme under grant agreement No 101070537.

<p align="center">
    <img src="https://crosscon.eu/sites/crosscon/themes/crosscon/images/eu.svg" width=10% height=10%>
</p>

<p align="center">
    <img src="https://crosscon.eu/sites/crosscon/files/public/styles/large_1080_/public/content-images/media/2023/crosscon_logo.png?itok=LUH3ejzO" width=25% height=25%>
</p>
