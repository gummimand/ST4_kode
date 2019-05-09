clear all;
close all;
%% Import data:
filename = 'data/data_raw3.txt';
delimiterIn = ',';
headerlinesIn = 1;
A = importdata(filename,delimiterIn,headerlinesIn);
data_raw = A.data;

filename = 'data/data_filtered3.txt';
delimiterIn = ',';
headerlinesIn = 1;
A = importdata(filename,delimiterIn,headerlinesIn);
data_filtered = A.data;

Fs = 50;


%% Actual processing and plotting of raw data
Nr = (size(data_raw,2)/1);
tr = (0:Nr-1)/Fs;

zacc=zeros(1,size(data_raw,2)/1);

for i=1:1:(size(data_raw,2))
    zacc = [zacc(2:end) data_raw(i)];
end


%% Processing and plotting of filtered data
Nf = (size(data_filtered,2)/1);
tf = (0:Nf-1)/Fs;

zaccF=zeros(1,size(data_filtered,2)/1);

for i=1:1:(size(data_filtered,2))
    zaccF = [zaccF(2:end) data_filtered(i)];
end

%% plot
subplot(2,1,1)
plot(tr,zacc)
title("Plot of raw accelerometer data");
legend('z')

subplot(2,1,2)
plot(tf,zaccF)
title("Plot of filtered accelerometer data");
legend('z')

%% Frekvensspektrum

frekr = Fs*(0:Nr-1)./(Nr);
frekf = Fs*(0:Nf-1)./(Nf);

Fzacc = abs(fft(zacc));
FzaccF = abs(fft(zaccF));

figure;
subplot(2,1,1)
stem(frekr,Fzacc)
title("Plot of raw accelerometer spectrum");
legend('z')

subplot(2,1,2)
stem(frekf,FzaccF)
title("Plot of filtered accelerometer spectrum");
legend('z')
