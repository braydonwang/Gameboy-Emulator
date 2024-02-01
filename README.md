<br />
<div align="center">
  <img src="https://github.com/braydonwang/Gameboy-Emulator/assets/16049357/fdef12cd-5d93-488d-9151-9a81d921c839" height="150"/>


  <h3 align="center">Gameboy Emulator</h3>
  <p align="center">Experience nostalgia by playing hundreds of beloved classic Nintendo Gameboy games from your own computer</p>
</div>

<!-- ABOUT THE PROJECT -->

## About The Project
**What's an emulator?**

An emulator is a software program or hardware device that allows one computer system to **behave** like another computer system. In simpler terms, an emulator is like a magical **translator** for your computer to "imitate" the functions of another system. An emulator needs to understand the inner workings of the guest system in order to _trick_ the software into thinking it's running on the actual system. They're commonly used for running old software on new hardware, or in our case, playing games from older consoles on modern devices. 

**Why build a Gameboy Emulator?**

Gameboy's have always been a staple in handheld gaming devices played and loved by millions of people worldwide. Although we were born in a generation that has Nintendo Switches and PlayStations, we still want to know what it's like to have owned a Nintendo Gameboy. **Low-level programming** was also something that we were all interested and curious to learn about at the time. Building a Gameboy Emulator not only teaches about the Gameboy itself, but also gives a glimpse into **computer architecture** and how computers operate in general. 

### Features:

- Displays B/W pixel tiles as UI that reacts to user input 
- Supports MBC1 mapper and cartridge battery to save and reload game states
- Allows for concurrency through built-in timer class that accounts for CPU cycles
-  
- 

### Built With

As **performance** is a key consideration for emulation projects, emulators are often written in low-level languages. C is a low-level language that allows developers to write highly-performant code, is easily portable, and provides direct access to system resources like memory. It's also a language that all of us learned in school and are fairly comfortable with.

- ![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
- ![CMake](https://img.shields.io/badge/CMake-%23008FBA.svg?style=for-the-badge&logo=cmake&logoColor=white)

<!-- GETTING STARTED -->

### References
- All-in-one comprehensive Gameboy development manual: https://gbdev.io/pandocs/
- Gameboy CPU instruction set: https://www.pastraiser.com/cpu/gameboy/gameboy_opcodes.html
- Collection of Gameboy ROMs for testing: https://github.com/retrio/gb-test-roms
- Test for Gameboy pixel processing unit (PPU): https://github.com/mattcurrie/dmg-acid2
- List of popular Gameboy ROMs to download for free: https://www.emulatorgames.net/roms/gameboy/

## Getting Started
1. Clone this repository
```
git clone https://github.com/braydonwang/Gameboy-Emulator.git
```
2. Install the latest versions of the following dependencies:
```
- SDL2
- SDL2-TTF
- Check
- build-essentials

* Note that on Mac, instead of using Homebrew to install SDL2 and SDL2-TTF, follow these setup instructions:
https://www.studyplan.dev/sdl-dev/sdl-setup-mac
```

3. Create a new 'build' folder in the root directory of the project and navigate to it
```
mkdir build
cd build
```

4. Run the following command:
```
cmake ..
```

5. Make the project and verify that no errors are thrown
```
make
```

6. Run any of the test ROMs or Gameboy games by specifying its relative path
```
gbemu/gbemu ../roms/<FILE_NAME>.gb
```


## Inside Look

**1. Legends of Zelda**

<img width="1512" alt="image" src="https://github.com/braydonwang/Gameboy-Emulator/assets/16049357/850b8295-a586-40c4-a3ad-3fe9343fcc9d">


**2. Super Mario Land**

<img width="1504" alt="image" src="https://github.com/braydonwang/Gameboy-Emulator/assets/16049357/8f3609c4-28e4-45f2-923f-d20d8bdb7fe6">


**3. Tetris**

<img width="1512" alt="image" src="https://github.com/braydonwang/Gameboy-Emulator/assets/16049357/5da06906-e20e-46ec-b52e-92e7126532cd">


## Next Steps

- [x] Add support for cartridge battery to allow saving game state
- [x] Provide detailed documentation to important parts of the code
- [ ] Add support for audio components
- [ ] Look into supporting other MBC mappers
- [ ] Look into supporting Gameboy Color registers (CGB)
