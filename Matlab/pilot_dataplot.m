clear all;
close all;
%% Import data:
filename = 'data/withGyro/data_7rounds.txt';
delimiterIn = ',';
headerlinesIn = 1;
A = importdata(filename,delimiterIn,headerlinesIn);
data = A.data;
Fs = 50;
N = (size(data,2)/6);
t = (0:N-1)/Fs;
n = 2;

%% Actual processing and plotting of data
xacc=zeros(1,size(data,2)/6);
yacc=zeros(1,size(data,2)/6);
zacc=zeros(1,size(data,2)/6);

xgyr=zeros(1,size(data,2)/6);
ygyr=zeros(1,size(data,2)/6);
zgyr=zeros(1,size(data,2)/6);


for i=1:6:(size(data,2))
    zacc = [zacc(2:end) data(i)];
    yacc = [yacc(2:end) data(i+1)];
    xacc = [xacc(2:end) data(i+2)];    
    
    zgyr = [zgyr(2:end) data(i+3)];
    ygyr = [ygyr(2:end) data(i+4)];
    xgyr = [xgyr(2:end) data(i+5)];
end

%plot(t,xacc)
hold on
%plot(t,yacc)
plot(t,zacc)
title("Plot of raw accelerometer data");
%legend('x','y','z')
legend('z')

figure;

plot(t,xgyr)
hold on
plot(t,ygyr)
plot(t,zgyr)
title("Plot of raw gyroskop data");
legend('x','y','z')

%% Filtering of data bandpass + moving average
Wnbp = [0.5 15]./(Fs/2);
[Bbp,Abp] = butter(n,Wnbp,'bandpass');

zacc = filtfilt(Bbp,Abp, zacc);
yacc = filtfilt(Bbp,Abp, yacc);
xacc = filtfilt(Bbp,Abp, xacc);

zgyr = filtfilt(Bbp,Abp, zgyr);
ygyr = filtfilt(Bbp,Abp, ygyr);
xgyr = filtfilt(Bbp,Abp, xgyr);

figure;
%plot(t,xacc)
hold on
%plot(t,yacc)
plot(t,zacc)
title("Plot of filtered accelerometer data");
%legend('x','y','z')
legend('z')

figure;
plot(t,xgyr)
hold on
plot(t,ygyr)
plot(t,zgyr)
title("Plot of filtered gyroskop data");
legend('x','y','z')

%% Moing average
% zacc_movmean = movmean(abs(zacc), 25);
% 
% figure;
% plot(t,zacc_movmean)
% title("Plot of filtered and movmean of accelerometer data");
% %legend('x','y','z')
% legend('z')




%% Envelope (rect + lowpass)
Wnlp = 1/(Fs/2);
[Blp,Alp] = butter(n,Wnlp,'low');

zacc = filtfilt(Blp,Alp, abs(zacc));
yacc = filtfilt(Blp,Alp, abs(yacc));
xacc = filtfilt(Blp,Alp, abs(xacc));

zgyr = filtfilt(Blp,Alp, abs(zgyr));
ygyr = filtfilt(Blp,Alp, abs(ygyr));
xgyr = filtfilt(Blp,Alp, abs(xgyr));

figure;
%plot(t,xacc)
hold on
%plot(t,yacc)
plot(t,zacc)
title("Plot of filtered and envelope of accelerometer data");
%legend('x','y','z')
legend('z')

figure;
plot(t,xgyr)
hold on
plot(t,ygyr)
plot(t,zgyr)
title("Plot of filtered and envelope of gyroskope data");
legend('x','y','z')



N/50