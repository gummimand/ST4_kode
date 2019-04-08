
%% Programbeskrivelse 
% 03/04-19
%Formålet med dette program er at generere en tabel med de korrekte tidsstempler for hvornår et bib skal afspilles under YOYO IR1 test. 
%Yderste for-løkke har til formål at kontrollere testens level mens
%inderste for-løkke har til formål at kontrollere antal runder under hvert level. 
%Der er til dette formål opstillet en tabel som i 1. række definerer
%testens levels (denne er på nuværende tidspunkt nødvendig). I anden række defineres antallet af runder tilhørende
%levelet i den pågældende kolonne. I tredie og fjerde række forfindes
%tidsintervallerne for hhv. total løbetid for den enkelte runde og
% midtvejstid (med 1 decimal, ved nedrunding). 

clc 
clear

%% initialiser tabeller 

lvl=[5 9 11 12 13 14 15 16 17 18 19 20 21 22 23;... %level (defineret ved YOYO IR1)
    1 1 2 3 4 8 8 8 8 8 8 8 8 8 8;... % antal runder tilhørende levelet i samme kolonne 
    14.4 12.5 11.1 10.7 10.3 9.9 9.6 9.3 9.0 8.7 8.5 8.2 8.0 7.8 7.6;... %total løbetid for det pågældende level
    7.2 6.2 5.5 5.3 5.1 4.9 4.8 4.6 4.5 4.3 4.2 4.1 4.0 3.9 3.8];  %midtvejsløbetid

level=1; %initierende level - skal bruges i yderste forløkke
round=1; % initierende runde - skal bruges i inderste forløkke
IR1=([0]); %definerer IR1 som tabel. Sikrer at første arrayplads er 0 -> skal bruges til startbib 
rest=10; %konstant pausetid
extra_beep_delay=0.4; % for at opnå dobbeltbib ved level-skift

%% Opret tabel 

for (level=level:length(lvl)) %yderste forløkke. Går fra 1 til længden af tabellen lvl hvilket svarer antallet af levels
    
      for (round=round:lvl(2,level)) %inderste forløkke. Går fra 1 til værdien i række 2 for det pågældende level defineret i yderste forløkke
          if(round==1) % genererer dobbeltbip ved levelskift
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1))+extra_beep_delay];
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1)-1)+lvl(4,level)];  
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1)-2)+lvl(3,level)]; 
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1))+rest]; 
            
          else 
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1))+lvl(4,level)]; % "length(IR1)+1" for arraypladsen i enden af den seneste arrayplads. "IR1(1,length(IR1)" for at opnå værdien på sidste plads hvortil midtvejstiden for det pågældende level adderes -> giver akkumuleret tid. 
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1)-1)+lvl(3,level)]; % tilsvarende ovenstående, men med "length(IR1)-1" da den totale tid ikke skal lægges til midtvejstiden, men til tiden før denne (nødvendigt for at få de afrundede decimaler til alligevel at passe)
            IR1(1,length(IR1)+1)=[IR1(1,length(IR1))+rest]; % samme princip men med pausetiden adderet 
            
          end 
      end
  
    round=1; %vigtigt at round resettes til 1!
end 


%% print tabel 

sprintf('%.1f, ',IR1(1,:))