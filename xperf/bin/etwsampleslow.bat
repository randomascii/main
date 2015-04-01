@echo Setting the ETW sampling profiler frequency to 10,000,000/40,000 samples
@echo per second, also known as one sample every 4.0 ms, or about 250
@echo samples per second.
xperf -setprofint 40000 cached
