@echo Setting the ETW sampling profiler frequency to 10,000,000/10,000 samples
@echo per second, also known as one sample every 1.0 ms, or about 1,000
@echo samples per second.
xperf -setprofint 10000 cached
