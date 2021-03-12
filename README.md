# VG2-vaerstasjonsprosjekt
Dette er et gammel C++-prosjekt jeg styrte med som hobby i VG2. Jeg er velkjent med at koden er suboptimal, og gjerne kunne trengt en oppfriskning. Grunnet alle feilene jeg oppdaget i mine design kommer jeg likevel ikke til å gjenoppta dette. Prosjektet ble for omstendelig. Dette prosjektet ligger her kun for å demonstrere programmeringserfaring.

Den nyeste versjonen av prosjektet er UniterGiOppFlere. Navnet stammer fra en kombinasjon av generell misnøye og at prosjektet er en kombinasjon av et værmåler-prosjekt og rf-sender-prosjekt (samme chip skulle håndtere begge)  
  
Prosjektet bestod av 3 egendesignede PCB-er bestående av en mikrokontroller, batteri, solcelle-ladekrets og rf-transmitter. I tillegg kom en server bestående av to tvillingmikrokontrollere (grunnet at hver ikke hadde nok pins), ladekrets, rf-transmitter, (div værmålerutstyr som vindmåler osv.), hvilket var designet til å koples opp mot en raspberry pi.  
  
En grov beskrivelse av det egendesignede kretskortet er som følger: (se gjerne bilde)  
Den kvadratiske store chippen er en 8-bit atmega328p mikrokontroller. Denne ble programmert med et programmeringskretskort jeg kunne kople til pins markert med gnd,tx og rx.  
Den lille rektangulære chippen ved siden av mikrokontrolleren er en 128kb EEPROM (lagringschip)  
Det påloddede kretskortet bestående av svært små komponenter er en kommersiell rf-chip (ikke designet av meg), hvilket kommuniserer med mikrokontrolleren gjennom SPI.  
Alle komponentene til høyre for dette tilhører en ladekrets med kontakter for solcellepanel (hvitt rektangel)  
På undersiden er det store hvite rektangelet en forholdsvis kraftig LED, og den minste er en temperatur-og-fuktighets sensor.  
  
Prosjektet ble terminert da at jeg etter utallige timer debugging oppdaget en loddefeil på ett av kretskortene (en kan skimte på bildet av mikrokontrollerens pins ikke er fullstendig koplet med loddetinn). Jeg mistenker dette hadde en stor rolle i hvordan sendesystemet hadde enormt pakketap (rundt 80%). I tillegg ble filene programmert svært lite orbjektorientert (langt mer prosedyreorientert), noe som gjorde de svært uoversiktlige. Kombinert med begynnelsen av mer krevende skolefag besluttet jeg det var best å gi opp.  
  
Her er en liste forbedringer jeg planla å gjøre. Jeg sliter selv med å si nøyaktig hva de betydde.  
*ack flag lik shortrequest flag - gi rettigheter  
*Avslutningsbyte ID = ConnectionByte ID (spørsmålstegn)  
*Bekreft at første ACK faktisk blir sent  
*Benyt HeaderID ikke egen ID  
*BlockEnd for neste i rekke  
*Cap for regnsensor  
*Duplikater av data  
*i2c claim  
*Lengde på siste melding  
*Listen mode  
*Mange feil mottakelser for videresender oppdaget  
*MeasuredLength ikke det man tror, len  
*Mottaker måler mengde, != mod 256  
*Registrer meldinger for ACK - som recvfromack  
*Resend problem  
*Send melding om å sende mer data  
*Siste send før bytt - medfører kresj!!!!  
*Sov hvis ingen melding oppdaget  
