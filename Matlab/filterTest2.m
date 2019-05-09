function filterTest(comport)
filename = 'data/data_filtered2.txt';
delimiterIn = ',';
headerlinesIn = 1;
A = importdata(filename,delimiterIn,headerlinesIn);
data = A.data;

zacc=zeros(1,size(data,2)/1);
for i=1:1:(size(data,2))
    zacc = [zacc(2:end) data(i)];
end
y = int16(zacc);

Fs  =   50;            % Sampling Frequency
Ts  =   1/Fs;           % Sampling Periode
L = size(y,2);                % Number of samples

N = 2;                  % Filter order
Fcut = [0.5 15];               % Cut-off frequency

[B,A] = butter(N,Fcut./(Fs/2),'bandpass') % Filter coefficients in length N+1 vector B

t = [0:L-1]*Ts;         % time array
A_m = 80;               % Amplitude of main component
F_m = 2;                % Frequency of main component
P_m = 80;               % Phase of main component

Gain = 1;               % Overall gain

% Math formula for the composition of a sine with a given amplitude,
% frequency and phase shift
y_m = A_m*sin(2*pi*F_m*t - P_m*(pi/180));

A_s = 40;               % Amplitude of secondary component
F_s = 20;               % Frequency of secondary component
P_s = 20;               % Phase of secondary component

% Math formula for the composition of a sine with a given amplitude,
% frequency and phase shift
y_s = A_s*sin(2*pi*F_s*t - P_s*(pi/180)); % pi/180 er for at få radianer

% sum of main and secondary components with a common gain
y_sum = Gain*(y_m + y_s);

%y = int16(y_sum);                % typecast to a signed 8 bit value

y_filt = int16(filter(B,A,y));   % filtered data (incl. typecast)

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
Serial_port_object = serial(comport);           % create Serial port object
set(Serial_port_object,'baudrate',115200)       % set baudrate 
set(Serial_port_object,'InputBufferSize',2*L)     % set InputBufferSize to length of data
set(Serial_port_object,'OutputBufferSize',2*L)    % set OutputBufferSize to length of data
try
    % Try to connect serial port object to device
    fopen(Serial_port_object)
catch
    % Display error message if connection fails and return
    disp(lasterr)
    return
end

fwrite(Serial_port_object,y,'int16');            % send out data as int8
data = fread(Serial_port_object,L,'int16');      % read back data as int8 (fread returns data as a double)
data = int16(data);                              % typecast to a signed 8 bit value
fclose(Serial_port_object)                      % close Com Port

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
% Draw the original and the filtered data %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
figure(1)
subplot(2,1,1)
hold off
plot(t,y)
hold on
plot(t,y_filt,'r')
plot(t,y_filt,'ro')
whos
plot(t,data,'k.')
ylabel('Amplitude')
legend('y','y filt (PC)','y filt (PC)','y filt (EPS32)')

subplot(2,1,2)
hold off
plot(t,data'-y_filt)
hold on
xlabel('time')
ylabel('ESP32 - PC')
