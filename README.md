## PUF-based Authentication Proof of Concept Implementation

This repository hosts the first proof of concept implementation of the PUF-based
authentication service developed as part of the [CROSSCON](https://crosscon.eu/)
project. This further contributed to the Deliverable D3.2: CROSSCON Open
Security Stack – Initial Version.

## Overview

CROSSCON is dedicated to developing a unified software stack tailored for
resource-constrained IoT devices. In alignment with this objective, we are
introducing a series of innovative, trusted services aimed at bolstering the
security of IoT devices and their services. This repository hosts the
implementations of our PUF-based authentication services, as detailed in
Deliverable D3.1.

This repository features implementations of three distinct solutions. While the
current versions are initial and subject to future modifications throughout the
project's duration, they represent a solid foundation for development and
testing.

Each folder in the repository corresponds to a project and includes one
authentication service. For each service, we provide example datasets that
contain various PUF challenges along with corresponding enrollment data. This
setup facilitates understanding and interaction with the authentication
mechanisms we have developed.

The proof of concept implementation was conducted and tested on the LPC55S69-EVK
development board from NXP.

## Running the examples

Each folder contains an independent project developed using NXP's MCUXpresso
IDE. The simplest way to run these projects is to import them into MCUXpresso
and utilize the accompanying toolchain to flash the software onto the device.
Alternatively, the projects can be imported into Visual Studio Code, where the
MCUXpresso SDK extension can be used to facilitate development and deployment.
Each implementation includes both the authentication and verification processes
directly on the device. Verification often involves interactions to retrieve
information from public storage; for simplicity, we provide exemplary data to
demonstrate the functionality. Specifically, this includes Path data 1-8, for
PAVOC and PAWOS as well as an ID. These Path data correspond to the enrollment
of a chain or a respective one-time signature pair, along with a path to an
exemplary Merkle tree. This Merkle tree contains more than one million enrolled
challenge and response pairs. The ID is the root of the Merkle Tree containing
all enrolled samples of the Prover. When you run the example, an authentication
message is first generated. This message is then passed to the verification
algorithm to confirm its authenticity.

## ZK-PUF

To run ZK-PUF, begin by opening the main function located in the source folder
under `LPC55S69_ZK-PUF.c`. This file includes an example that demonstrates all
the provided functionalities. Initially, the necessary parameters are set up
using the `init_ECC` function. These include the elliptic curve group (for our
example, SECP256R1), a secondary group generator, and a storage location for
the commitment C, created during the commitment phase. Once initialization is
complete, the device is enrolled using the `enroll_ECC` function. This function
returns an integer indicating whether the enrollment was successful. Enrollment
is generally a one-time process conducted in a secure setting, but for
demonstration purposes, we provide all necessary functionality directly on the
development board. After enrollment, the device is ready to use ZK-PUF for
authentication by calling the `authenticate_ECC` function. This process writes
the generated zero-knowledge proof into the designated *proof, result_v, and
result_w* variables and returns an integer indicating the success of the proof
generation. Finally, the proof is passed to the `verify_ECC` function to verify
the authenticity of the proof. To modify the parameters used in this example,
you can edit the constants in the `constant.h` file.

## PAVOC

The main function of PAVOC is located under `source -> LPC55S69_PAVOC.c`. This
example features a set of eight distinctive challenges and an array specifying
the chain length for each individual hash chain. Additionally, it includes the
Merkle root, which serves as the prover's ID in our system, along with a
distinctive path for each enrolled chain.
To run the example, you must first enroll the challenges using the `enroll`
function. This function creates an individual hash chain for each provided
challenge based on the PUF response and stores the chain ends in the responses
array.
For authentication, the `authenticate` function is used. It requires the
specific challenge and its current time step. In a real-world setup, this time
step should be monotonically decreased, reflecting the time synchronization
between the prover and the verifier. Once the time slot is considered expired,
the used chain element must be released using the `revealChainElement` function.
The `verify` function is then used to confirm successful authentication. It
takes the revealed chain element, the time step, and an inclusion path for the
Merkle tree. The function first verifies that the revealed chain element was
actually used to authenticate the message. It then recreates the public chain
end formed during the enrollment phase by hashing the revealed chain element up
the chain to its end. Finally, it verifies the inclusion of the used element in
the Merkle tree using the provided path. In a practical setup, this path could
be retrieved from a public storage solution and does not need to be stored on
the device. This simplification is only to demonstrate the functionality of this
approach.


## PAWOS

The main function of PAVOC is located under `source -> LPC55S69_PAWOS.c`. This
example, similar to PAVOC, provides eight distinctive exemplary challenges along
with corresponding Merkle roots and paths. However, unlike PAVOC, which uses
hash chains, this implementation employs a one-time signature scheme for
authentication.
Initially, the challenges are enrolled using the `enroll` function. This
function returns the public value for each one-time signature pair,
corresponding to one PUF response. To authenticate, a one-time signature is
generated using the provided challenge. The verifier then verifies the received
one-time signature by reconstructing the public value from it. Additionally, it
checks that the used value is correctly enrolled with the claimed prover by
verifying the Merkle path.
If both checks are successful, the function returns `0`; otherwise, it returns
`1`. For each exemplary challenge, we provide the exemplary Merkle path for the
given root. Note that if you change the challenge used in the authentication,
the corresponding path must be used for authentication to succeed.

## License

See LICENSE file.

## Acknowledgments

The work presented in this repository is part of the
[CROSSCON project](https://crosscon.eu/) that received funding from the European
Union’s Horizon Europe research and innovation programme under grant agreement
No 101070537.

<p align="center">
    <img src="https://crosscon.eu/sites/crosscon/themes/crosscon/images/eu.svg" width=10% height=10%>
</p>

<p align="center">
    <img src="https://crosscon.eu/sites/crosscon/files/public/styles/large_1080_/public/content-images/media/2023/crosscon_logo.png?itok=LUH3ejzO" width=25% height=25%>
</p>
