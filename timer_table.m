
%% Programbeskrivelse 
% 03/04-19
%Form�let med dette program er at generere en tabel med de korrekte tidsstempler for hvorn�r et bib skal afspilles under YOYO IR1 test. 
%Yderste for-l�kke har til form�l at kontrollere testens level mens
%inderste for-l�kke har til form�l at kontrollere antal runder under hvert level. 
%Der er til dette form�l opstillet en tabel som i 1. r�kke definerer
%testens levels (denne er p� nuv�rende tidspunkt n�dvendig). I anden r�kke defineres antallet af runder tilh�rende
%levelet i den p�g�ldende kolonne. I tredie og fjerde r�kke forfindes
%tidsintervallerne for hhv. total l�betid for den enkelte runde og
% midtvejstid (med 1 decimal, ved nedrunding). 

clc 
clear

%% initialiser tabeller 

lvl=[5 9 11 12 13 14 15 16 17 18 19 20 21 22 23;... %level (defineret ved YOYO IR1)
    1 1 2 3 4 8 8 8 8 8 8 8 8 8 8;... % antal runder tilh�rende levelet i samme kolonne 
    14.4 12.5 11.1 10.7 10.3 9.9 9.6 9.3 9.0 8.7 8.5 8.2 8.0 7.8 7.6;... %total l�betid for det p�g�ldende level
    7.2 6.2 5.5 5.3 5.1 4.9 4.8 4.6 4.5 4.3 4.2 4.1 4.0 3.9 3.8];  %midtvejsl�betid

level=1; %initierende level - skal bruges i yderste forl�kke
round=1; % initierende runde - skal bruges i inderste forl�kke
IR1=([0]); %definerer IR1 som tabel. Sikrer at f�rste arrayplads er 0 -> skal bruges til startbib 
rest=10; %konstant pausetid
extra_beep_delay=0.4; % for at opn� dobbeltbib ved level-skift

%% Opret tabel 

for (level=level:length(lvl)) %yderste forl�kke. G�r fra 1 til l�ngden af tabellen lvl hvilket svarer antallet af levels
    
      for (round=round:lvl(2,level)) %inderste forl�kke. G�r fra 1 til v�rdien i r�kke 2 for det p�g�ldende level defineret i yderste forl�kke
          if(round==1) % genererer dobbeltbip ved levelskift
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1))+extra_beep_delay];
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1)-1)+lvl(4,level)];  
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1)-2)+lvl(3,level)]; 
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1))+rest]; 
            
          else 
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1))+lvl(4,level)]; % "length(IR1)+1" for arraypladsen i enden af den seneste arrayplads. "IR1(1,length(IR1)" for at opn� v�rdien p� sidste plads hvortil midtvejstiden for det p�g�ldende level adderes -> giver akkumuleret tid. 
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1)-1)+lvl(3,level)]; % tilsvarende ovenst�ende, men med "length(IR1)-1" da den totale tid ikke skal l�gges til midtvejstiden, men til tiden f�r denne (n�dvendigt for at f� de afrundede decimaler til alligevel at passe)
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1))+rest]; % samme princip men med pausetiden adderet 
            
          end 
      end
  
    round=1; %vigtigt at round resettes til 1!
end 


%% print tabel 

sprintf('%.1f, ',IR1(1,:))