@echo Setting the ETW sampling profiler frequency to 10,000,000/1,221 samples
@echo per second, also known as one sample every 0.1221 ms, or about 8190
@echo samples per second.
xperf -setprofint 1221 cached
