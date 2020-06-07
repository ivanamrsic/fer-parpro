# fer-parpro

## First homework

### Compile & run

* `mpicc -o philosopher ./philosopher.c`
* `mpirun -np {num-of-processors} ./philosopher`

### Task

* Zadatak: uporabom MPI-a izraditi simulaciju raspodijeljenog problema n filozofa.

Slično uobičajenoj inačici problema, model obuhvaća n filozofa koji sjede za okruglim stolom na kojemu se nalazi hrana. Na raspologanju je n vilica, od kojih za potrebe prehrane svaki filozof mora koristiti dvije. Svaki filozof koristi točno određene vilice (njegovu 'lijevu' i njegovu 'desnu'), tj. svaku pojedinačnu vilicu mogu koristiti samo dva susjedna filozofa. Za razliku od uobičajenog rješenja sa zajedničkim spremnikom, filozofi moraju biti implementirani kao procesi koji komuniciraju isključivo razmjenom poruka (raspodijeljena okolina). Iz istog razloga, vilice ne stoje na stolu nego se uvijek nalaze kod nekog od filozofa (procesa). Program se mora moći pokrenuti u proizvoljnom broju procesa (n > 1), a ispis je potrebno prilagoditi tako da svaki proces/filozof ispisuje promjene stanja uz uvlačenje teksta (tabs) proporcionalno indeksu procesa.

```
Proces(i)
{    misli (slucajan broj sekundi);        // ispis: mislim
    nabavi vilice;                // ispis: trazim vilicu (indeks)
    jedi (slucajan broj sekundi);        // ispis: jedem
}
```

Svaka od n vilica može biti čista ili prljava, te se u jednom trenutku može nalaziti samo kod jednog filozofa (naravno). Na početku, sve su vilice prljave. Također, vilice su na početku podijeljene tako da se svaka vilica, koju mogu dijeliti dva susjedna filozofa, nalazi kod filozofa s nižim rednim brojem (indeksom procesa). Slijedom navedenog, filozof s indeksom 0 na početku ima dvije vilice, a filozof s indeksom n-1 niti jednu. Svi filozofi slijede ova pravila:

* filozof jede ako je gladan i ako ima obje vilice (bez obzira jesu li čiste ili prljave ;)
* nakon jela, obje korištene vilice postaju prljave.
* ako filozof želi jesti, šalje zahtjeve za vilicama koje nisu kod njega i čeka na odgovor.
* ako filozof ne jede, a postoji zahtjev za prljavu vilicu koja se nalazi kod njega, vilicu čisti i šalje je susjedu.
* svaki filozof pamti zahtjeve za vilicama koje je dobio od svojih susjeda.
* dok filozof misli, udovoljava svakom zahtjevu za vilicom koja je trenutno kod njega.
* Iz navedenih pravila je vidljivo da filozof ne udovoljava zahtjevu za čistom vilicom - zahtjev će biti udovoljen tek kad vilica postane prljava (nakon jela). Isto tako, ukoliko filozof misli (trenutno nije gladan), obvezan je odmah (što prije) udovoljiti zahtjevima drugih filozofa.

```
Proces(i)
{    misli (slucajan broj sekundi);                    // ispis: mislim
        i 'istovremeno' odgovaraj na zahtjeve!            // asinkrono, s povremenom provjerom (vidi nastavak)
    dok (nemam obje vilice) {
        posalji zahtjev za vilicom;                // ispis: trazim vilicu (i)
        ponavljaj {
            cekaj poruku (bilo koju!);
            ako je poruka odgovor na zahtjev        // dobio vilicu
                azuriraj vilice; 
            inace ako je poruka zahtjev            // drugi traze moju vilicu
                obradi zahtjev (odobri ili zabiljezi);
        } dok ne dobijes trazenu vilicu;
    }
    jedi;                                // ispis: jedem
    odgovori na postojeće zahtjeve;                    // ako ih je bilo
}
```

"Istovremeno" odgovaranje na zahtjeve potrebno je izvesti tako da filozof povremeno provjerava postoji li neki zahtjev (nema poruka) upućen njemu. Ova funkcionalnost može se postići funkcijom MPI_Iprobe koja trenutno (neblokirajući) provjerava postoji li neka poruka upućena promatranom procesu (predavanja, poglavlje 2.8). Tek nakon što funkcija detektira postojanje dolazne poruke, ista se treba primiti s MPI_Recv. U našem primjeru, interval provjeravanja postojanja dolaznih poruka nije kritičan i može biti jednak 1 sekundu.


## Second homework

### Task

* Zadatak: uporabom MPI-a ostvariti program za igranje uspravne igre "4 u nizu" (connect 4) za jednog igrača (čovjek protiv računala).

* Opis igre: igra je istovjetna igri križić-kružić u kojoj je cilj napraviti niz od 4 igračeva znaka, s tom razlikom da se odvija na 'uspravnom' 2D polju u kojemu se novi znak može staviti samo na polje ispod kojega već postoji neki znak ili se stavlja na dno polja ('ajmo reć' uz gravitaciju). Standardne dimenzije igračeg polja su 6 polja u visinu i 7 u širinu, mada je veličina proizvoljna. Zbog jednostavnosti, može se pretpostaviti da polje nije ograničeno u visinu, dok ograničenje u širinu mora postojati zbog ograničavanja broja mogućih poteza iz zadanog stanja

* Opis slijednog algoritma: neka metoda rješavanja bude djelomično pretraživanje prostora stanja, u obliku stabla, do neke zadane dubine od trenutnog stanja. Dakle, ne pokušavamo naučiti strategiju, već se za svaki potez računala obavlja pretraga podstabla i odabire sljedeće stanje (brute force pristup). Za svako se stanje u stablu određuje vrsta:

    * stanje je 'pobjeda' ako računalo ima 4 u nizu (vrijednost 1);
    * stanje je 'poraz' ako igrač ima 4 u nizu (vrijednost -1);
    * inače, stanje je neutralno, a vrijednost će ovisiti o stanjima u podstablu (ako se podstabla pretražuju).

* HINT: potragu za 4 u nizu treba obaviti samo sa polja posljednjeg odigranog poteza.

* Nakon pretraživanja stabla do zadane dubine, primjenjuju se sljedeća rekurzivna pravila:

    1. ako je neko stanje 'pobjeda' i ako se u njega dolazi potezom računala, tada je i nadređeno stanje također 'pobjeda' (jer računalo iz nadređenog stanja uvijek može pobijediti, potezom koji vodi u stanje pobjede);
    2. ako je stanje 'poraz' i ako se u njega dolazi potezom igrača (protivnika), tada je i nadređeno stanje 'poraz' (jer iz nadređenog stanja igrač može jednim potezom pobijediti);
    3. ako su sva podstanja nekog stanja 'pobjeda' ili 'poraz', tada je i nadređeno stanje iste vrste.

Osim ovim pravilima, svaki se mogući potez računala (odnosno stanje u koje se tim potezom dolazi) ocjenjuje promatranjem broja i dubine pobjedničkih stanja u podstablu u koje taj potez vodi. Mjera kvalitete poteza (jednog podstabla) definira se rekurzivno kao razlika broja pobjedničkih stanja i broja poraza na nekoj dubini, podijeljenih sa umnoškom broja mogućih poteza na toj dubini, odnosno:
(broj_pobjeda_u_dubini_n - broj_poraza_u_dubini_n)/(broj_mogućih_poteza)
Broj mogućih poteza se može smatrati konstantnim (npr. 7) uz pretpostavku o neograničenosti polja u visinu, a u stvarnim uvjetima potrebno je uzeti u obzir samo moguće poteze (ako je neki stupac popunjen). Računalo tada odabire onaj potez koji ne vodi u stanje 'poraz' (ako ima izbora) a koji ima najveću vrijednost (vrijednosti su po opisanoj definiciji u intervalu [-1,1]). Eventualna dodatna pojašnjenja dana su na predavanjima.

####Ostvarenje paralelnog algoritma

Program treba imati minimalno tekstno sučelje u obliku prikaza stanja polja i upita igrača o potezu. Računanje vrijednosti pojedinog poteza treba načiniti raspodijeljeno, a minimalna dubina pretraživanja stabla n je 4 (složenost je 7^n). Minimalni broj zadataka paralelnog algoritma je broj mogućih poteza (7), no taj broj je potrebno povećati (npr. dijeljenjem pri većoj dubini) poradi boljeg ujednačavanja opterećenja po procesorima.

> Algoritam u kojemu postoji stalan broj od najviše 7 zadataka (radnika) nije prihvatljiv!
