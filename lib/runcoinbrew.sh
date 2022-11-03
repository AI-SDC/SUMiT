ECHO $PWD
./coinbrew fetch Clp --latest-release 
./coinbrew fetch Osi --latest-release 
./coinbrew build Osi --verbosity 2 --parallel-jobs 4 --build-dir build -p dist
./coinbrew build Clp --verbosity 2 --parallel-jobs 4 --build-dir build -p dist

