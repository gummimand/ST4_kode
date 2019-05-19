Serial_port_object = serial('com6');           % create Serial port object
set(Serial_port_object,'baudrate',115200)       % set baudrate 
try
    % Try to connect serial port object to device
    fopen(Serial_port_object)
catch
    % Display error message if connection fails and return
    disp(lasterr)
    return
end

pause(1)
fprintf(Serial_port_object,'%s','S');


BytesAtPort = get(Serial_port_object,'BytesAvailable');
while(~BytesAtPort)
    BytesAtPort = get(Serial_port_object,'BytesAvailable');
end

tic
varighed = 0;
i = 2;

while (1)
    data = fread(Serial_port_object,1,'uint8');
    varighed(i) = varighed(i-1) + toc
    tic; 
    i = i+1;
    
    BytesAtPort = get(Serial_port_object,'BytesAvailable');
end

fclose(Serial_port_object);