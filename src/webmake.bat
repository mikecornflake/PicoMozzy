nasm -fcoff z80cpu.asm -o z80cpux.o

gcc -c -W -Wall -O3 6545.c              %1 %2
gcc -c -W -Wall -O3 z80pio.c            %1 %2
gcc -c -W -Wall -O3 z80cpu.c            %1 %2
gcc -c -W -Wall -O3 genmod.c            %1 %2
gcc -c -W -Wall -O3 interf.c -IS_WEB    %1 %2

gcc -c -W -Wall -O3 modules.c           %1 %2
gcc -c -W -Wall -O3 configer.c          %1 %2
gcc -c -W -Wall -O3 debmaloc.c          %1 %2
gcc -c -W -Wall -O3 beefile.c           %1 %2

gcc -W -Wall -O3 %1 %2 -DIS_WEB mbee.c *.o -o mbee.exe -lalleg
rem gcc -W -Wall -O3 %1 %2 -DIS_WEB -DDEBUGMODE mbee.c *.o -o mbee.exe -lalleg
rem gcc -W -Wall -O3 %1 %2 -DIS_WEB -DDEBUG_MALLOC mbee.c *.o -o mbee.exe -lalleg
rem gcc -W -Wall -O3 %1 %2 -DIS_WEB -DDEBUGMODE -DDEBUG_MALLOC mbee.c *.o -o mbee.exe -lalleg

