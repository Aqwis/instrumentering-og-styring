Oppsett av mexopencv
===================

Denne guiden antar at du installerer i hjemmemappen din. Dersom du installerer system-wide blir alt mye lettere. mexopencv krever OpenCV versjon 2.4. Sjekk ut fra Github og kompiler med

    cmake -D MATLAB_ROOT_DIR=<matlab path> -D CMAKE_INSTALL_PREFIX=<home-mappe> .

for eksempel

    cmake -D MATLAB_ROOT_DIR=/usr/lib/matlab2012a/ -D CMAKE_INSTALL_PREFIX=/home/birkeland/trygvwii .

og

    make

    make install

Sjekk så mexopencv ut fra Github og kompiler med

    make MATLABDIR=/usr/lib/matlab2012a

lib-mappen du installerte må inkluderes i LD_LIBRARY_PATH. Skriv

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:<hjemmemappe>/lib

For å starte Matlab:

    LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libstdc++.so.6 matlab

Problemet her er at Matlab, smart som den er, bruker sin egen, ødelagte libstdc++.so.6. Dersom du har root-tilgang kan du i stedet erstatte ```/usr/lib/matlab2012a/bin/glnxa64/libstdc++.so.6``` med libstdc++.so.6-filen over.

I Matlab, skriv følgende for å kunne bruke cv-funksjonene:

    addpath('<hjemmemappe>/mexopencv');

Du skal nå kunne bruke cv-funksjonene. Dersom du får feilmeldinger som handler om manglende filer, sjekk https://github.com/kyamagu/mexopencv.
