### Building Instructions

# Starting off
Before running anything, you need to load in the compiler. Since we are compiling for Intel FPGA using Intel's OneAPI for FPGA, run

`module load ofs/agilex`

# Compiling
There are many different "ways" to compile.
* `make sim` simulates entirely in software. This is the fastest to ensure functional correctness
* `make link` This partially builds, and will get you some data in the reports on usage overall (more on reports later), useful for resource monitoring
* `make fpga` This is the full compile. This takes hours, 50 minutes minimum even for a small refresh. Try not to run this until you are so sure you are right. **ALSO CTRL+C DOES NOT KILL EVERYTHING ALL THE TIME, do ps -u 18643_team and kill anything named quartus if you need to cancel it**

# Viewing the reports
After making link of FPGA, you can view the reports. Go to build/<buildname>/reports and open the html file (you'll want to do this on another machine you actually have a screen for, maybe try X11 forwarding firefox from this one if you REALLY need to, also DO NOT USE VNC CMU will get super mad) 