# ttune

Trilby hat SDR. Raspberry pi hat http://www.kinetic.co.uk/Trilby.php
Also contains 

# install on raspberry

    apt-get install libasound2-dev


    wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.50.tar.gz


#  To run, without trilby hardware

   make nospi

#  To run on other linux than raspberry
  sudo apt-get install libfftw3-dev 

  mkdir build
  cd build
  cmake ..
  make
  ./ttunevg -t -s
Keypress should be done in terminal window, not openvg window 


#  Errors in Trilby sample firmware API document,
´´´
  Squelch is on register 0xf5
  Mute is on register 0xf6
´´´
