1. Compile
You can use make to build the two programs.
$ make
cc -Wall -ggdb -pedantic -c shared.c
cc -Wall -ggdb -pedantic oss.c shared.o -o oss
cc -Wall -ggdb -pedantic palin.c shared.o -o palin

oss and palin are the two executables created.
We will use only oss. palin is the child process started from oss.
To clean the source code use clean.
$ make clean
rm -f ./oss ./user output.txt common.o palin.out nopalin.out

2. Execution
To see available options use the -h option
$ ./oss -h
Usage: oss [-h] [-n 4] [-s 2] [-i input.txt] [-i output.txt]
        -h       Usage
        -n x    Max users to start
        -s x     Max users running at the same time
        -i filename     Input file with strings
        -o filename     Output file

Tpo execute the program with default option, just type ./oss
$ ./oss -n 10 -s 10
[287:283626000] 12917 ENTER critical section 0
[287:299948000] 12917 INSIDE critical section 0
[287:326193000] 12918 ENTER critical section 0
[287:578372000] 12920 ENTER critical section 0
[287:581602000] 12926 ENTER critical section 1
[287:590426000] 12926 INSIDE critical section 1
[287:641127000] 12924 ENTER critical section 1
[287:808368000] 12923 ENTER critical section 1
[287:821288000] 12919 ENTER critical section 0
[287:957209000] 12925 ENTER critical section 1
[288:113398000] 12921 ENTER critical section 0
[289:669420000] 12922 ENTER critical section 1
[1347:108025000] 12917 LEFT critical section 0
[1347:110414000] 12918 INSIDE critical section 0
[1347:348334000] 12926 LEFT critical section 1
[1347:389741000] 12924 INSIDE critical section 1
[2331:499690000] 12918 LEFT critical section 0
[2331:501610000] 12920 INSIDE critical section 0
[2331:832297000] 12924 LEFT critical section 1
[2331:833745000] 12923 INSIDE critical section 1
[3371:244495000] 12920 LEFT critical section 0
[3371:302133000] 12919 INSIDE critical section 0
[3371:516151000] 12923 LEFT critical section 1
[3371:518632000] 12925 INSIDE critical section 1
[4404:848992000] 12921 INSIDE critical section 0
[4404:879115000] 12919 LEFT critical section 0
[4405:60753000] 12925 LEFT critical section 1
[4405:79473000] 12922 INSIDE critical section 1
[5506:395558000] 12921 LEFT critical section 0
[5506:654636000] 12922 LEFT critical section 1

$ cat output.txt
[1821:959267000] Pali 13707 finished
[3000:820005000] Pali 13708 finished
[4267:531065000] Pali 13709 finished
[5519:952657000] Pali 13711 finished
[6778:184794000] Pali 13710 finished
[0:0] Child 1 with PID=13707 STARTED
13707 1 car

[0:1000] Child 2 with PID=13708 STARTED
13708 2 1230321

[0:2000] Child 3 with PID=13709 STARTED
13709 3 murder

[0:3000] Child 4 with PID=13710 STARTED
13710 4 mom

[0:4000] Child 5 with PID=13711 STARTED
13711 5 computer

[1822:21213000] PID 13707 done (exit 0)
[3000:866550000] PID 13708 done (exit 0)
[4267:573582000] PID 13709 done (exit 0)
[5520:222000] PID 13711 done (exit 0)
[6778:236685000] PID 13710 done (exit 0)
[6778:236685000] oss done (exit 0)
