clear all;
close all;
%% Import data:
filename = 'data_gyroTest2.txt';
delimiterIn = ',';
headerlinesIn = 1;
A = importdata(filename,delimiterIn,headerlinesIn);
data = A.data;
Fs = 50;
N = (size(data,2)/6);
t = (0:N-1)/Fs;

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

plot(t,xacc)
hold on
plot(t,yacc)
plot(t,zacc)
title("Plot of accelerometer data");
legend('x','y','z')

figure;

plot(t,xgyr)
hold on
plot(t,ygyr)
plot(t,zgyr)
title("Plot of gyroskop data");
legend('x','y','z')

%%
