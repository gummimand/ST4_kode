filterTest2('com5');

%% Coefficients
Fs  =   50;            % Sampling Frequency
Ts  =   1/Fs;           % Sampling Periode
L = 200;                % Number of samples

N = 2;                  % Filter order
Fcut = 1;               % Cut-off frequency
[B,A] = butter(N,Fcut./(Fs/2),'low') % Filter coefficients in length N+1 vector B