# <img src="html/images/logotst2.gif" alt="HighWire" />

Web browser for the Atari ST, TT, Falcon and FireBee.

Runs on any ST compatible machine, no GDOS required, and renders using SpeedoGDOS, NVDI or fVDI if you have one.

You'll need to install a TCP/IP stack (e.g. STinG, STiK2, MiNTnet) to get online.

[![Build Status](https://github.com/freemint/highwire/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/freemint/highwire/actions)

## Install

- Download the latest snapshot (or an archived version) below
- Extract the zip file to your hard disk
- Rename the build that matches your machine to `HIGHWIRE.PRG`:
  - `.000` — any 68000: ST, STE, Mega ST/STE
  - `.030` — 68030: Falcon, TT (works with or without an FPU)
  - `.03F` — 68030 with a 68881/2 FPU fitted (same, but faster)
  - `.040` / `.060` — accelerated machines with FPU
  - `.v4e` — ColdFire (FireBee)
- Rename the `.OVL` file in the `MODULES` folder that matches your TCP/IP stack to `NETWORK.OVL`, e.g. `STING.OVL` for STinG
- Run `HIGHWIRE.PRG`

## Downloads

- [Latest snapshot](https://atari.joska.no/snapshots/highwire/highwire-latest.zip)
- [Archive](https://atari.joska.no/snapshots/highwire/)
