data = csvread('Runs/TimeVsFileSize11_34_481722773895.csv');
data2 = csvread('Runs/FinalPatternVsCount5_37_53222732047.csv');
data3 = csvread('Runs/ThreadsVsThroughput5_37_53222732047.csv');

figure 

scatter(data(1:end, 1), data(1:end, 2));
xlabel('Processing Time (milliSeconds)');
ylabel('File size (Bytes)');

figure 

stem(data2(1:end, 1), data2(1:end, 2));

axis([30 100 0 inf])
xlabel('Pattern Size');
ylabel('File Count');

%ghetto way of displaying throughput trendlines
figure 
prevVal = 0;
count = 0;
for idx = 1:numel(data3(1:end, 1))+1
    element = data3(idx);
    if(prevVal > element)
        plot(data3(idx-count:idx-1, 1), data3(idx-count:idx-1, 2));
        count = 0;
        hold on
    end
    prevVal = data3(idx);
    count = count + 1;
end
%plot(data3(1:end, 1), data3(1:end, 2));

xlabel('Threads');
ylabel('Speedup');