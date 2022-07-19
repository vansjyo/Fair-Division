# Fair-Division

#### Steps to run the code
* Clone the repository (download zip or clone)
* Set the number of samples to generate, upper and lower bounds on valuations, #agents and #items in `sampleGenerator.txt`
* Run `g++ sampleGenerator.cpp` and `./a` to generate random samples. 
* Run `make -f makefile.mak`. If make command not supported, run `g++ main.cpp helper.cpp output.cpp`
* Run the exexutive file generated - `./main`
* To log the console output to a log file, run `./main | tee "/<fileLocation>/output.txt"`
