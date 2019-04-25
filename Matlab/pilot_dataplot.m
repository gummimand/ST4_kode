clear all;
close all;
%% Import data:
filename = 'data.txt';
delimiterIn = ',';
headerlinesIn = 1;
A = importdata(filename,delimiterIn,headerlinesIn);
data = A.data;

%% Actual processing and plotting of data
x=zeros(1,size(data,2)/3);
y=zeros(1,size(data,2)/3);
z=zeros(1,size(data,2)/3);

for i=1:3:(size(data,2))
    z = [z(2:end) data(i)];
    y = [y(2:end) data(i+1)];
    x = [x(2:end) data(i+2)];    
end

plot(x)
hold on
plot(y)
plot(z)
title("Plot of data");
legend('x','y','z')

%%
