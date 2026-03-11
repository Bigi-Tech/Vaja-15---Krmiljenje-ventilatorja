# Vaja 15: Merjenje temperature z NTC in Nucleo L476RG

## Opis projekta
Cilj te naloge je bil sprogramirati ARM mikrokrmilnik (razvojna plošča Nucleo L476RG) s pomočjo orodja STM32CubeIDE in HAL knjižnic. Mikrokrmilnik meri temperaturo okolice, rezultate pošilja na računalnik, krmili hitrost ventilatorja in upravlja z varnostnim alarmom.

Glavne funkcionalnosti sistema:
* Branje analogne vrednosti napetosti na uporovnem delilniku (NTC sonda in fiksni upor) preko ADC vhoda.
* Preračunavanje ADC vrednosti v temperaturo ter pošiljanje podatkov preko UART protokola na odjemalec (Putty).
* Linearno krmiljenje hitrosti DC ventilatorja (preko tranzistorja) z uporabo PWM signala sorazmerno z naraščanjem temperature.
* Proženje alarma z LED diodo, če temperatura preseže kritično mejo 45°C. Ob alarmu LED dioda utripa s frekvenco 2 Hz s pomočjo SysTick časovnika.
* Izklop alarma z uporabniško tipko (zunanja prekinitev - EXTI), vendar strogo pod pogojem, da je temperatura padla pod kritični nivo.

---

## Konfiguracija pinov (Pinout)
Mikrokrmilnik STM32L476RGTx ima pine konfigurirane na naslednji način:
* **PC0 (ADC3_IN1):** Analogni vhod za branje vrednosti na delilniku (NTC in 6800 Ohm upor).
* **PA2 / PA3 (USART2_TX / RX):** Serijska komunikacija za prenos podatkov na PC z baud rate 115200.
* **PA8 (TIM1_CH1):** Izhod za strojno generiranje PWM signala (frekvenca 10 kHz) za krmiljenje tranzistorja ventilatorja.
* **PA5 (LED):** Digitalni izhod za opozorilno LED diodo.
* **PC13 (User_EXTI):** Vhod z omogočeno zunanjo prekinitvijo (EXTI15_10) ob padajoči flanki za resetiranje alarma.

---

## Delovanje programa
1. **Branje temperature:** Program periodično (vsako 1 sekundo) zažene ADC pretvorbo na pinu PC0. Surova vrednost se zajame po končani pretvorbi. Program izračuna upornost NTC sonde in jo z uporabo Steinhart-Hartove enačbe ter izmerjenimi logaritmi pretvori iz Kelvinov v stopinje Celzija.
2. **Serijski izpis:** Izračunana temperatura (v formatu celega števila) in surova ADC vrednost se pretvorita v besedilni niz in pošljeta preko UART na serijski terminal Putty.
3. **Krmiljenje ventilatorja:** Glede na izračunano temperaturo se po linearni enačbi določi Duty-Cycle (od 0 do 100 %). Pri 25°C ventilator miruje (0 %), pri 100°C pa doseže maksimalno moč. Vrednost se neposredno zapiše v CCR1 register časovnika TIM1.
4. **Alarmni sistem:** * V kolikor temperatura doseže ali preseže 45°C in alarm še ni aktiven, se sproži zastavica `Alarm_OFF = 0`.
   * Ko je alarm sprožen, program skrbi za utripanje LED diode na pinu PA5 (menjava stanja vsakih 250 ms).
   * S pritiskom na modro tipko (PC13) se sproži EXTI zunanja prekinitev. V prekinitveni rutini `HAL_GPIO_EXTI_Callback` se preveri, ali je temperatura varna (`T2int < 45`). Le v tem primeru se LED ugasne in postavi `Alarm_OFF = 1`. Zaradi mehanskih odbojev tipke je v rutini uporabljena programska zakasnitev.

---

## Slika Pinouta iz CubeIDE
<img width="727" height="579" alt="image" src="https://github.com/user-attachments/assets/93f990d6-0cb6-4fd0-99f8-238bf923ab16" />

## Odgovori na vprašanja

**Je izpis temperature pravilen?**
Da. Program z uporabo vhodnih podatkov, NTC koeficientov (A, B, C) in funkcije logaritma (`log()`) iz knjižnice `<math.h>` ustrezno linearizira napetost in izpiše realno vrednost temperature v stopinjah Celzija.

**Se LED dioda prižge pri segrevanju NTC upora preko postavljene meje?**
Da. Program ob doseženi temperaturi 45°C uspešno vstopi v pogoj za alarm in aktivira utripanje LED diode preko SysTick časovnika.

**Se je LED dioda ugasnila ob pritisku na gumb?**
Da, vendar izključno ob izpolnjenem varnostnem pogoju. Če pritisnemo tipko, medtem ko je temperatura še vedno previsoka (45°C ali več), sistem pritisk ignorira. LED se ugasne šele, ko se NTC dovolj ohladi in takrat pritisnemo uporabniško tipko.

---

## Komentar na delovanje
Zasnova celovitega sistema, ki integrira ADC, strojni PWM za pogon zunanjih bremen in zunanje prekinitve, deluje zanesljivo in stabilno. Ločitev zanke za branje na eno sekundo in asinhrono reševanje prekinitve gumba ter proženje utripanja LED s pomočjo glavnega sistemskega časa omogoča mikrokrmilniku gladko izvajanje ostalih nalog brez nepotrebnega čakanja in uporabe blokirnih funkcij, kot je `HAL_Delay()`.
