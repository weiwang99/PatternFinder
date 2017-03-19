data = csvread('Runs/TimeVsFileSize11_34_481722773895.csv');
data2 = csvread('Runs/FinalPatternVsCount5_37_53222732047.csv');
data3 = csvread('Runs/ThreadsVsThroughput5_37_53222732047.csv');

figure 

scatter(data(1:end, 1), data(1:end, 2));

xlabel('Processing Time (milliSeconds)');
ylabel('File size (Bytes)');

figure 

plot(data2(1:end, 1), data2(1:end, 2));

xlabel('Pattern Size');
ylabel('File Count');        count = 0;
        hold on