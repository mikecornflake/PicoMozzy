rem nasm -fcoff z80cpu.asm -o z80cpux.o

rem - stuff to do here
rem gcc -c -W -Wall -O3 6545.c              %1 %2
rem gcc -c -W -Wall -O3 z80pio.c            %1 %2
rem gcc -c -W -Wall -O3 z80cpu.c            %1 %2
rem gcc -c -W -Wall -O3 genmod.c            %1 %2
rem gcc -c -W -Wall -O3 interf.c -DIS_DJGPP %1 %2

rem gcc -c -W -Wall -O3 modules.c           %1 %2
rem gcc -c -W -Wall -O3 configer.c          %1 %2
rem gcc -c -W -Wall -O3 debmaloc.c          %1 %2
rem gcc -c -W -Wall -O3 beefile.c           %1 %2

gcc -W -Wall -O3 %1 %2 -DIS_DJGPP mbee.c *.o -o mbee.exe -lalleg
rem gcc -W -Wall -O3 %1 %2 -DIS_DJGPP -DDEBUGMODE mbee.c *.o -o mbee.exe -lalleg
rem gcc -W -Wall -O3 %1 %2 -DIS_DJGPP -DDEBUG_MALLOC mbee.c *.o -o mbee.exe -lalleg
rem gcc -W -Wall -O3 %1 %2 -DIS_DJGPP -DDEBUGMODE -DDEBUG_MALLOC mbee.c *.o -o mbee.exe -lalleg

