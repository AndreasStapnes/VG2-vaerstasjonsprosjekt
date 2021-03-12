# VG2-vaerstasjonsprosjekt
Dette er et gammel C++-prosjekt jeg styrte med som hobby i VG2. Jeg er velkjent med at koden er suboptimal, og gjerne kunne trengt en oppfriskning. Grunnet alle feilene jeg oppdaget i mine design kommer jeg likevel ikke til å gjenoppta dette. Prosjektet ble for omstendelig. Dette prosjektet ligger her kun for å demonstrere programmeringserfaring.

Den nyeste versjonen av prosjektet er UniterGiOppFlere. Navnet stammer fra en kombinasjon av generell misnøye og at prosjektet er en kombinasjon av værmåler-prosjektet og rf-sender-prosjektet (samme chip skulle håndtere begge)

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
