**_X-PLC_**

**_Programming Language_**

	

[[TOC]]

*1. Abstract*

Lo scopo del documento è di descrivere il linguaggio di programmazione del sistema programmabile X-PLC.

Il sistema X-PLC può essere programmato in due modalità differenti, utilizzando l’opportuna opzioni presente nel Designer

* LADDER

* Instruction list

Il LADDER è una modalità di programmazione grafica a contatti tipica del mondo della automazione.

L’ Instruction list, al contrario, permette di programmare le logiche di automaizone dei sistemi X-PLC attravreso un linguaggio di programmazione a metà tra il basic e lo structureed text utilizzato anch’esso in ambito automazione.

In questo documento focalizzeremo la nostra attenzione sul linguaggio di programmazione.

*2. Instruction list*

Per instruction list si intende il nome del linguaggio di programmazione che può essere utilizzato per la progettazione della logica di funzionamento del X-PLC core PLC.

Un programma X-PLC è separato in tre sezioni:

* Definizione dei simboli mnemonici che verranno utilizzati

* Sezione del MAIN

* Sezione delle PROCEDURES

BEGIN_PROGRAM

		

SYMBOLS

...

…

END_SYMBOLS

BEGIN_MAIN

...

...

END_MAIN

BEGIN_PROCEDURES

…

…

END_PROCEDURES

END_RPOGRAM

## **_2.1 Symbols_**

Nella sezione dei simboli, racchiussa fra le parole chiave **SYMBOLS** ed **END_SYMBOLS**, possiamo creare le associazioni fra nomi mnemonici e indirizzi PLC; ogni volta che utilizziamo il simbolo definito, in fase di pre compilazione, verrà eseguita una conversione del mnemonico in indirizzo reale.

Per definire un simbolo si utilizza la seguente sintassi:

…

#SYMBOL_BOOL_1		= 	M10.1

…

## **_2.2 Main_**

La sezione **_main_**, compresa fra le parole chiavi **BEGIN_MAIN** e **END_MAIN**, contiene le linee di programma che verranno eseguite a partire dalla prima fino all’ultima (a meno di salti presenti nel codice).

Una linea di programma rappresenta una porzione di codice che esegue una singola azione; deve essere racchiusa tra le parole chiave **BEGIN** ed **END**.

Un riga di programma può essere identificata da un identificativo numerico che deve essere unico all’interno di tutto il programma. Nelle funzioni di GOTO occorre utlizzare l’0idnetificativo per saltare alla riga desiderata.

…

BEGIN(:99)

…

END

…

Una linea di programma è da interndersi come una parte di logica iniziale che attiva o meno una azione finale.

Una riga può iniziare con una delle seguenti istruzioni

<table>
  <tr>
    <td>LOAD  #PAR</td>
    <td>Funzione che carica il valore del paramtero #PAR nell’accumulatore.

Il parametro #PAR può essere del tipo
Intero
Byte
Digitale
Doppia word
</td>
  </tr>
  <tr>
    <td>LOADN  #PAR</td>
    <td>Funzione che carica nell’accumulatore il valore negato del parametro #PAR.

Il parametro #PAR può essere del tipo
Digitale
</td>
  </tr>
  <tr>
    <td>AND #PAR</td>
    <td>Inizia una sequnza di operatori logici, controllando che #PAR sia vero (diverso da 0).
</td>
  </tr>
  <tr>
    <td>ANDN #PAR</td>
    <td>Inizia una sequenza di operatori logici controllando che #PAR sia falso (uguale a 0)
</td>
  </tr>
  <tr>
    <td>SET #PAR</td>
    <td>Imposta a vero la memoria definita in #PAR senza controllare la logica di attivazione.
</td>
  </tr>
  <tr>
    <td>RESET #PAR</td>
    <td>Imposta a falso la memoria definita in #PAR senza controllare la logica di attivazione.
</td>
  </tr>
  <tr>
    <td>GOTO :<LABEL></td>
    <td>Salta direttamente alla linea contrassegnata con la label <LABEL>.
</td>
  </tr>
  <tr>
    <td></td>
    <td></td>
  </tr>
</table>


**_Esempi di linee di programma._**

**_1. _**

Riga di programma che assegna il valore della memoria definita nel simbolo #SYMBOL_1 alla memoria definita dal simbolo #OUT_1.

…

BEGIN

	LOAD	#SYMBOL_1

	STORE	#OUT_1

END

…

**_2. _**

Riga di programma che esegue un operazione di AND logico fra le tre memorie definite dai simboli #SYMBOL_1, #SYMBOL_2 e #SYMBOL_3. Nel caso in cui l’AND sia verificato (ovvero tutte le memorie sono vere) allora esegue il SET della memoria rappresentata dal simbolo #OUT_1, in caso contrario non fa nessuna azione

…

BEGIN

	AND	#SYMBOL_1

	AND	#SYMBOL_2

	AND	#SYMBOL_3

	SET	#OUT_1

END

…

**_3. _**

Riga di programma che esegue un operazione di OR logico fra le due memorie definite dai simboli #SYMBOL_1 e #SYMBOL_2. Nel caso in cui l’OR sia verificato (ovvero almeno una delle due memorie sia vera) allora esegue il RESET della memoria rappresentata dal simbolo #OUT_1, in caso contrario non fa nessuna azione

…

BEGIN

	AND	#SYMBOL_1

	OR	#SYMBOL_2

	RESET	#OUT_1

END

…

**_4. _**

Riga di che permette di eseguire un salto diretto ad un’altra linea contrassegnata da una LABEL

…

BEGIN

	GOTO	:100

END

…

...

BEGIN(:100)

	AND 	#PAR

	SET	#OUT_1

END

## **_2.3 Procedures_**

La sezione procedure, delimitata dalle parole chiave **BEGIN_PROCEDURES** ed **END_PROCEDURES**, permette di definire delle procedure che possono essere richiamate utilizzando la chiamata **CALL.**

Ogni procedura, delimitata dalle parole chiave **PROCEDURE** ed **END_PROCEDURE** rappresenta un insieme di righe di programma.

Ogni procedura deve essere identificata da un identificativo numerico univoco utilizzato in fase di chiamata con la funzione **CALL.**

BEGIN_PROCEDURES

...

PROCEDURE(:1)

		…

BEGIN

AND 		#PAR

SET		#OUT_1

END

…

END_PROCEDURE

..

END_PROCEDURES

## **_2.4 Lista istruzioni_**

Vediamo di seguito la lista di tutte le istruzioni supportate dal linguaggio ed in che modo si possono utilizzare.

Occore rimarcare il concetto che tutte le operazioni vengono effettuatre sul valore assunto da un accumulatore che deve essere opportunamente inizializzato da una delle istruzioni 

### **2.4.1 LOAD**

*Sintassi*

**_	LOAD	#SYMBOL_**

*Descrizione*

Tale istruzione può essere posizionata all’inizio di una riga di programma e consente di inizializzare l’accumulatore con il valore della memoria definita dal simbolo # SYMBOL.

La LOAD è in grado di accettare tutte i possibili indirizzi del PLC.

*Esempi*

//

// Riflette il valore della memoria M100.0 nella prima uscita a relè

BEGIN

	LOAD 		M100.0

	STORE		U0.0

END

### **2.4.2 LOADN**

*Sintassi*

**_	LOADN	#SYMBOL_**

*Descrizione*

Tale istruzione può essere posizionata all’inizio di una riga di programma e consente di inizializzare l’accumulatore con il valore negato della memoria definita dal simbolo # SYMBOL.

La LOADN è in grado di accettare solo di indirizzi di variabili digitali.

*Esempi*

//

// Riflette il valore negato della memoria M100.0 nella prima uscita a relè

BEGIN

	LOADN 		M100.0

	STORE		U0.0

END

### **2.4.3 AND**

*Sintassi*

**_	AND		#SYMBOL_**

*Descrizione*

Tale istruzione può essere posizionata all’inizio di una riga di programma e consente di inziare una sequenza di condizioni logiche che, se verificate porteranno alla esecuzione della relativa azione. In caso contrario l’azione non verrà eseguita.

La AND è in grado di accettare solo di indirizzi di variabili digitali.

*Esempi*

//

// Viene settata l’uscita a relè quando sia M100.0 che I0.0 sono entrambi a vero

BEGIN

	AND 		M100.0

	AND 		I0.0

	SET		U0.0

END

//

// Viene settata l’uscita a relè quando o M100.0 o I0.0 sono vere

BEGIN

	AND 		M100.0

	OR 		I0.0

	SET		U0.0

END

### **2.4.4 ANDNOT**

*Sintassi*

**_	ANDNOT		#SYMBOL_**

*Descrizione*

Tale istruzione può essere posizionata all’inizio di una riga di programma e consente di inziare una sequenza di condizioni logiche con il valore negato del simbolo. 

La ANDNOT è in grado di accettare solo di indirizzi di variabili digitali.

*Esempi*

//

// Viene settata l’uscita a relè quando M100.0 è falsa e I0.0 è vera

BEGIN

	ANDNOT	M100.0

	AND 		I0.0

	SET		U0.0

END

//

// Viene settata l’uscita a relè quando o M100.0 è falsa o I0.0 è vera

BEGIN

	ANDNOT	M100.0

	OR 		I0.0

	SET		U0.0

END

### **2.4.5 OR**

*Sintassi*

**_	OR		#SYMBOL_**

*Descrizione*

Tale istruzione può essere posizionata all’interno di una sequenza logica.

La OR è in grado di accettare solo di indirizzi di variabili digitali.

*Esempi*

//

// Viene settata l’uscita a relè quando o M100.0 o I0.0 sono vere

BEGIN

	AND 		M100.0

	OR 		I0.0

	SET		U0.0

END

### **2.4.6 ORNOT**

*Sintassi*

**_	ORNOT		#SYMBOL_**

*Descrizione*

Tale istruzione può essere posizionata all’interno di una sequenza logica. Implementa l’OR del valore negato della memoria rappresentata dal simbolo.

La ORNOT è in grado di accettare solo di indirizzi di variabili digitali.

*Esempi*

//

// Viene settata l’uscita a relè quando o M100.0 è vera o I0.0 è falsa.

BEGIN

	AND 		M100.0

	ORNOT		I0.0

	SET		U0.0

END

### **2.4.7 XOR**

*Sintassi*

**_	XOR		#SYMBOL_**

*Descrizione*

Tale istruzione può essere posizionata all’interno di una sequenza logica. Implementa l’OR esclusivo del valore della memoria rappresentata dal simbolo.

La XOR è in grado di accettare solo di indirizzi di variabili digitali.

*Esempi*

//

// Viene settata l’uscita a relè quando M100.0 è diversa da I0.0.

BEGIN

	AND 		M100.0

	XOR		I0.0

	SET		U0.0

END

### **2.4.8 ADD**

*Sintassi*

**_	ADD		#SYMBOL [<CONST>]_**

*Descrizione*

Tale istruzione consente di sommare al contenuto dell’accumultaore il valore della memoria indirizzata da #SYMBOL, oppure il valore presente nella constante CONST.

La ADD è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// riga che incrementa di 1 il valore del byte in memoria alla locazione 100.

BEGIN

	LOAD 		MB100

	ADD		1

	STORE		MB100

END

### **2.4.9 SUB**

*Sintassi*

**_	SUB		#SYMBOL [<CONST>]_**

*Descrizione*

Tale istruzione consente di sottrarre al contenuto dell’accumultaore il valore della memoria indirizzata da #SYMBOL, oppure il valore presente nella constante CONST.

La SUB è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// riga che decrementa di 1 il valore del byte in memoria alla locazione 100.

BEGIN

	LOAD 		MB100

	SUB		1

	STORE		MB100

END

### **2.4.10 MUL**

*Sintassi*

**_	MUL		#SYMBOL [<CONST>]_**

*Descrizione*

Tale istruzione consente di moltiplicare il contenuto dell’accumultaore per il valore della memoria indirizzata da #SYMBOL, oppure il valore presente nella constante CONST.

La MUL è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// riga che moltiplica per 2 il valore del byte in memoria alla locazione 100.

BEGIN

	LOAD 		MB100

	MUL		2

	STORE		MB100

END

### **2.4.11 DIV**

*Sintassi*

**_	DIV		#SYMBOL [<CONST>]_**

*Descrizione*

Tale istruzione consente di dividere il contenuto dell’accumultaore per il valore della memoria indirizzata da #SYMBOL, oppure il valore presente nella constante CONST.

La DIV è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// riga che divide per 2 il valore del byte in memoria alla locazione 100.

BEGIN

	LOAD 		MB100

	DIV		2

	STORE		MB100

END

### **2.4.12 EQ**

*Sintassi*

**_	EQ		#SYMBOL [<CONST>]_**

*Descrizione*

Tale istruzione consente di verificare se il valore dell’accumulatore è uguale al valore della memoria indirizzata da #SYMBOL oppure al valore assunto dalla costante CONST..

La EQ è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// Riga che controlla se il valore della memeoria alla locazione 100 è uguale a 10. 

// Nel caso di espressione verificata viene eseguito il salto alla riga 500.

// In caso contrario non si fa nulla.

BEGIN

	LOAD 		MB100

	EQ		10

	BEGIN_THEN

		GOTO	 :500

	END_THEN

	BEGIN_ELSE

		NOP

	END_ELSE

	NOP

END

### **2.4.13 NEQ**

*Sintassi*

**_	NEQ		#SYMBOL [<CONST>]_**

*Descrizione*

Tale istruzione consente di verificare se il valore dell’accumulatore è diverso dal valore della memoria indirizzata da #SYMBOL oppure al valore assunto dalla costante CONST..

La NEQ è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// Riga che controlla se il valore della memeoria alla locazione 100 è diverso da 10. 

// Nel caso di espressione verificata viene eseguito il salto alla riga 500.

// In caso contrario non si fa nulla.

BEGIN

	LOAD 		MB100

	NEQ		10

	BEGIN_THEN

		GOTO	 :500

	END_THEN

	BEGIN_ELSE

		NOP

	END_ELSE

	NOP

END

### **2.4.14 GT**

*Sintassi*

**_	GT		#SYMBOL [<CONST>]_**

*Descrizione*

Tale istruzione consente di verificare se il valore dell’accumulatore è maggiore del valore della memoria indirizzata da #SYMBOL oppure al valore assunto dalla costante CONST..

La GT è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// Riga che controlla se il valore della memoria alla locazione 100 è maggiore di 10. 

// Nel caso di espressione verificata viene eseguito il salto alla riga 500.

// In caso contrario non si fa nulla.

BEGIN

	LOAD 		MB100

	GT		10

	BEGIN_THEN

		GOTO	 :500

	END_THEN

	BEGIN_ELSE

		NOP

	END_ELSE

	NOP

END

### **2.4.15 GE**

*Sintassi*

**_	GE		#SYMBOL [<CONST>]_**

*Descrizione*

Tale istruzione consente di verificare se il valore dell’accumulatore è maggiore o uguale del valore della memoria indirizzata da #SYMBOL oppure al valore assunto dalla costante CONST..

La GE è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// Riga che controlla se il valore della memoria alla locazione 100 è maggioreo uguale a 10. 

// Nel caso di espressione verificata viene eseguito il salto alla riga 500.

// In caso contrario non si fa nulla.

BEGIN

	LOAD 		MB100

	GE		10

	BEGIN_THEN

		GOTO	 :500

	END_THEN

	BEGIN_ELSE

		NOP

	END_ELSE

	NOP

END

### **2.4.16 LT**

*Sintassi*

**_	LT		#SYMBOL [<CONST>]_**

*Descrizione*

Tale istruzione consente di verificare se il valore dell’accumulatore è minore del valore della memoria indirizzata da #SYMBOL oppure al valore assunto dalla costante CONST..

La LT è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// Riga che controlla se il valore della memoria alla locazione 100 è minore di 10. 

// Nel caso di espressione verificata viene eseguito il salto alla riga 500.

// In caso contrario non si fa nulla.

BEGIN

	LOAD 		MB100

	LT		10

	BEGIN_THEN

		GOTO	 :500

	END_THEN

	BEGIN_ELSE

		NOP

	END_ELSE

	NOP

END

### **2.4.17 LE**

*Sintassi*

**_	LE		#SYMBOL [<CONST>]_**

*Descrizione*

Tale istruzione consente di verificare se il valore dell’accumulatore è minore o uguale del valore della memoria indirizzata da #SYMBOL oppure al valore assunto dalla costante CONST..

La LE è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// Riga che controlla se il valore della memoria alla locazione 100 è minore o uguale a 10. 

// Nel caso di espressione verificata viene eseguito il salto alla riga 500.

// In caso contrario non si fa nulla.

BEGIN

	LOAD 		MB100

	LT		10

	BEGIN_THEN

		GOTO	 :500

	END_THEN

	BEGIN_ELSE

		NOP

	END_ELSE

	NOP

END

### **2.4.18 IN**

*Sintassi*

**_	IN		#SYMBOL1[<CONST1>]   #SYMBOL2[<CONST2>]_**

*Descrizione*

Tale istruzione consente di verificare se il valore dell’accumulatore è compreso nel range tra il valore della memoria indirizzata da #SYMBOL1 oppure al valore assunto dalla costante CONST1 e il valore della memoria indirizzata da #SYMBOL2 oppure al valore assunto dalla costante CONST2

La IN è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// Riga che controlla se il valore della memoria alla locazione 100 è compresa fra 10 e 100. 

// Nel caso di espressione verificata viene eseguito il salto alla riga 500.

// In caso contrario non si fa nulla.

BEGIN

	LOAD 		MB100

	IN		10 100

	BEGIN_THEN

		GOTO	 :500

	END_THEN

	BEGIN_ELSE

		NOP

	END_ELSE

	NOP

END

### **2.4.18 NIN**

*Sintassi*

**_	NIN		#SYMBOL1[<CONST1>]   #SYMBOL2[<CONST2>]_**

*Descrizione*

Tale istruzione consente di verificare se il valore dell’accumulatore non è compreso nel range tra il valore della memoria indirizzata da #SYMBOL1 oppure al valore assunto dalla costante CONST1 e il valore della memoria indirizzata da #SYMBOL2 oppure al valore assunto dalla costante CONST2

La NIN è in grado di accettare tutti gli indirizzi e le costanti.

*Esempi*

//

// Riga che controlla se il valore della memoria alla locazione 100 non è compresa 

// fra 10 e 100. 

// Nel caso di espressione verificata viene eseguito il salto alla riga 500.

// In caso contrario non si fa nulla.

BEGIN

	LOAD 		MB100

	NIN		10 100

	BEGIN_THEN

		GOTO	 :500

	END_THEN

	BEGIN_ELSE

		NOP

	END_ELSE

	NOP

END

### **2.4.19 GOTO**

*Sintassi*

**_	GOTO		:<LABEL>_**

*Descrizione*

Tale istruzione consente di eseguire un salto diretto alla linea di programma identificata da <LABEL>

La GOTO è in grado di accettare solo delle labels.

*Esempi*

//

// Riga che esegue il salto alla linea 500 in caso di variabile MB100.0 uguale a vero.

BEGIN

	AND 		MB100.0

	GOTO		:500

END

### **2.4.20 NOP**

*Sintassi*

**_	NOP	_**

*Descrizione*

Tale istruzione non esegue nulla.

*Esempi*

//

// Nel caso di righe di tipo IF, then else dobbiamo rimepire i rami non completi con delle NOP

BEGIN

	LOAD 		MB100

	NIN		10 100

	BEGIN_THEN

		GOTO	 :500

	END_THEN

	BEGIN_ELSE

		NOP

	END_ELSE

	NOP

END

### **2.4.21 STORE**

*Sintassi*

**_	STORE		#SYMBOL_**

*Descrizione*

Tale istruzione consente di copiare il valore presente nell’accumulatore nella memoria indiirzzata da #SYMBOL.

 

La STORE è in grado di accettare tutti i tipi di indirizzi.

*Esempi*

//

// Riga che assegna alla uscita a relè 5 il valore del bit 100.0

BEGIN

	LOAD 		MB100.0

	STORE		U0.5

END

### **2.4.22 SET**

*Sintassi*

**_	SET		#SYMBOL_**

*Descrizione*

Tale istruzione imposta a vero il valore della memoria indirizzata da #SYMBOL.

 

La SET è in grado di accettare i tipi di dati digiali.

*Esempi*

//

// Riga che setta l’uscita 5 quando il valore del bt 100.0 è vero.

BEGIN

	AND 		MB100.0

	SET		U0.5

END

### **2.4.23 RESET**

*Sintassi*

**_	RESET		#SYMBOL_**

*Descrizione*

Tale istruzione imposta a falso il valore della memoria indirizzata da #SYMBOL.

 

La RESET è in grado di accettare i tipi di dati digiali.

*Esempi*

//

// Riga che resetta l’uscita 5 quando il valore del bt 100.0 è vero.

BEGIN

	AND 		MB100.0

	RESET		U0.5

END

### **2.4.24 TOGGLE**

*Sintassi*

**_	TOGGLE		#SYMBOL_**

*Descrizione*

Tale istruzione esegue una negazione del valore della memoria indirizzata da #SYMBOL. Nel caso il valore sia vero lo imposta a falso e viceversa.

 

La TOGGLE è in grado di accettare i tipi di dati digiali.

*Esempi*

//

// Riga che esegue il toggle dell’uscita 5 quando il valore del bt 100.0 è vero.

BEGIN

	AND 		MB100.0

	TOGGLE	U0.5

END

## **_2.5 Timers_**

Possiamo defnire due tipologie di timer.

### **2.5.1 Timer a attivazione**

![image alt text](image_0.png)

Il timer ad attivazione è un timer che ha due ingressi ed una uscita:

* **Enable** che abilita il conteggio.

* **T** che definisce il tempo di conteggio.

* **Q** uscita digitale chesi attiva alla fine del conteggio.

Utilizzando il diagramma temporale degli ingressi/uscite capiamo il comportamento del timer

![image alt text](image_1.png)

*Esempio di utilizzo*

//

// Carica nel timer TON1 il valore di conteggio

BEGIN

	LOAD		1000

	STORE		TON1

END

//

// Attiva il conteggio quando il valore della variabile 100.0 è verà

BEGIN

	AND		M100.0

	SET		TON1

END

//

// Resetta il timer quando il valore della variabile 100.1 è vero

BEGIN

	AND		M100.1

	RESET		TON1

END

### **2.5.2 Timer a disattivazione**

![image alt text](image_2.png)

Il timer ad attivazione è un timer che ha due ingressi ed una uscita:

* **Enable** che abilita il conteggio.

* **T** che definisce il tempo di conteggio.

* **Q** uscita digitale chesi attiva alla fine del conteggio.

Utilizzando il diagramma temporale degli ingressi/uscite capiamo il comportamento del timer

![image alt text](image_3.png)

*Esempio di utilizzo*

//

// Carica nel timer TON1 il valore di conteggio

BEGIN

	LOAD		1000

	STORE		TOF2

END

//

// Attiva il conteggio quando il valore della variabile 100.0 è verà

BEGIN

	AND		M100.0

	SET		TOF2

END

//

// Resetta il timer quando il valore della variabile 100.1 è vero

BEGIN

	AND		M100.1

	RESET		TOF2

END

## **_2.6 Contatori_**

![image alt text](image_4.png)

Un contatore è un blocco funzionale che conta, incrementando oppure decrementando di una unita il proprio valore di conteggio, quando l’ingresso di Clock verifica determinate condizioni.

Un volta che il conteggio assume il valore Cnt preimpostato l’uscita Q del contatore si attiva fino a che non viene fatto un reset dello stesso.

I contatori si suddividono in

* Contatori a incremento sul fronte di salita del clock. In questo caso il conteggio viene incrementato ad ogni fronte di salita.

* Contatori a incremento sul livello alto del clock. In questo caso il conteggio viene incrementato ad ogni ciclo di PLC in cui il livello della variabile clock è alto.

* Contatori a incremento sul fronte di discesa del clock. In questo caso il conteggio viene incrementato ad ogni fronte di discesa.

* Contatori a incremento sul livello basso del clock. In questo caso il conteggio viene incrementato ad ogni ciclo di PLC in cui il livello della variabile clock è basso.

* Contatori a decremento sul fronte di salita del clock. In questo caso il conteggio viene decrementato ad ogni fronte di salita.

* Contatori a decremento sul livello alto del clock. In questo caso il conteggio viene decrementato ad ogni ciclo di PLC in cui il livello della variabile clock è alto.

## **_2.7 Impulsi_**

Un impulso è un modulo sotware che consente di verificare il cambio di stato di una variabile digitale da zero a uno oppure viceversa. In tal caso il valore dell’impulso rimmarrà a vero per tutto un ciclo di programma.

**Impulso di salita**: consente di controllare il fronte di salita di una variabile digitale

//

// Controllo lo stato dell’ingresso digitale 0

BEGIN

	LOAD		I0.0

	STORE		REP1

END

//

// Quando ho un fronte REP1 utilizzato come digitale assume valore vero

BEGIN

	AND		REP1

	SET		U0.0

END

**Impulso di discesa**: consente di controllare il fronte di discesa di una variabile digitale

//

// Controllo lo stato dell’ingresso digitale 0

BEGIN

	LOAD		I0.0

	STORE		FEP1

END

//

// Quando ho un fronte FEP1 utilizzato come digitale assume valore vero

BEGIN

	AND		FEP1

	SET		U0.0

END

## **_2.8 Controllo di flusso_**

Durante la progettazione della logica con il linguaggio di programmazione del X-PLC è possibile utilizzaere una serie di costrutti sintattici per la gestione del flusso di programma.

### **2.8.1 IF, THEN, ELSE**

*Sintassi*

**_	LOAD		#SYMBOL_**

**_	<OP>		#SYMBOL1_**

**_	BEGIN_THEN_**

**_		…_**

**_	END_THEN_**

**_	BEGIN_ELSE_**

**_		…_**

**_	END_ELSE_**

Sicuramente il costrutto universalmente adottato da tutti i linguaggi di programmazione è appunto l’IF che consente di eseguire un ramo di codice piuttosto che un altro a seconda della validaità o meno di una condizione boolenana.

Per valutare la validità di una condizione, occorre utilizzare l’accumulatore; come prima cosa dobbiamo caricare nell’accumultaore il valore della memoria o del simbolo che desideriamo controllare, ed in secondo luogo applichiamo l’operatore che ci interessa.

Abbiamo visto che i possibili operatori che possiamo utilizzare sono:

* [EQ](#heading=h.2jxsxqh) per stabilire se il valore dell’accumulatore è uguale al valore specificato a destra della condizione

* [NEQ](#heading=h.3j2qqm3) in caso di disugualizanza

* [GT](#heading=h.1y810tw) quando il valore dell’accumultaore deve essere maggiore del secondo operando, posizionato a destra della condizione

* [GE](#heading=h.4i7ojhp) maggiore o uguale

* [LT](#heading=h.2xcytpi) minore di

* [LE](#heading=h.1ci93xb) minore o uguale

* [IN](#heading=h.3whwml4) nel caso in cui vogliamo vedere si il valore dell’accumultaore si trova all’interno di un range (GT and LE)

* [NIN](#heading=h.2bn6wsx) per valutare se il valore dell’accumulatore non è all’interno di un range

Nel caso in cui la condizione è verificata, il programma passerà per il ramo BEGIN_THEN, END_THEN che può contenere anche più di una riga; in caso contrario il programma proseguirà nel ramo BEGIN_ELSE, END_ELSE.

*Esempio di utilizzo*

//

// Se il valore dell’ingresso digitale è 1 allora memorizzo 100

// nella memoria, altrimenti memorizzo 200.

BEGIN

	LOAD		I0.0

	EQ		True

	BEGIN_THEN

		//

		// Memorizzo 100

		BEGIN

			LOAD 		100

			STORE		MW210

		END

	END_THEN

	BEGIN_ELSE

		//

		// Memorizzo 200

		BEGIN

			LOAD		200

			STORE		MW210

		END

	END_ELSE

	//

	// Necessario per indicare che nessuna azione deve essere eseguita

	NOP

END

### **2.8.2 SWITCH, CASE**

*Sintassi*

**_	LOAD		#SYMBOL_**

**_	SWITCH_**

**_		CASE <0>_**

**_			…_**

**_		END_CASE_**

**_		CASE <1>_**

**_			…_**

**_		END_CASE_**

**_	_**

**_		…_**

**_		CASE <n>_**

**_			…_**

**_		END_CASE_**

**_	END_SWITCH_**

Durante la progetttazione di una logica sicuramente ci troveremop di fronte alla gestione di una macchiana a stati; in tal caso può essere utilizzato questo costrutto linguistico che ci permette di eseguire porzioni di codice diverso a seconda del valore che assume l’accumulatore.

All’interno delle sezioni CASE, END_CASE possiamo inserire anche più righe di codice.

*Esempio di utilizzo*

//

// A seconda del valore (0, 1 e 2 ) che 

BEGIN

	LOAD		#STATUS

	SWITCH

		CASE 0

			//

			// Chiamo la procedura 1

			CALL :1

		END_CASE

		CASE 1

			//

			// Chiamo la procedura 2

			CALL :2

		END_CASE

		CASE 2

			//

			// Chiamo la procedura 3

			CALL :3

		END_CASE

	END_SWITCH

	

	//

	// Necessario per indicare che nessuna azione deve essere eseguita

	NOP

END

## **_2.9 Indirizzamento_**

### **2.9.1 Memoria e I/O**

Vediamo come si possono defnire gli indirizzi dei simboli all’interno del PLC.

Gli indirizzi vengono dichiarati come la composizione di tre o quattro informazioni, a seconda del tipo di dato:

**_	<WHAT><TYPE><ADDRESS>[ . <BIT>]_**

**<WHAT>**

suddividiamo gli indirizzi in tre tipologie a seconda del contesto:

* Inidirizzi di **memoria interna**, identificati dalla lettera **M**

* Indirizzi per gli **ingressi**, identificati dalla lettera **I**

* Indirizzi per le **uscite**, identificati dalla lettera **U**

**<TYPE>**

Ogni indirizzo può rappresentare un tipo di dato ben preciso; la lista dei tipi di dato è la seguente:

* **Bit** senza alcun identificativo. Questa informazioni viene memorizzata in un bit.

* **Byte**, identificato dalla lettera **B. **Questa informazione viene memorizzata in un byte.

* **Word**, identificato dalla lettera **W. **Questa informazione viene memorizzata in due bytes.

* **Doppia Word**, identificata dalla lettera **D. **Questa informazione viene memorizzata in quattro bytes.

* **Reale**, identificato dalla lettera **R. **Questa informazione viene memorizzata in quattro bytes.

**<ADDRESS>**

Indirizzo a cui puntare.

Per la memoria possiamo arrivare fino a 4096.

Per gli I/O dipende dal dispositivo.

**<BIT>**

Indice del bit a cui fare riferimento in caso di digital. Il numero può variare da 0 a 7.

**_Esempi di indirizzamento_**

<table>
  <tr>
    <td>M100.3</td>
    <td>Indirizzo che punto ad un digitale in memoria al byte 100 e bit 3.
</td>
  </tr>
  <tr>
    <td>I0.1</td>
    <td>Indirizzo del secondo ingresso digitale.
</td>
  </tr>
  <tr>
    <td>U0.2</td>
    <td>Indirizzo della terza uscita a relè.
</td>
  </tr>
  <tr>
    <td>IR1</td>
    <td>Indirizzo della prima analogica. Essendo una analogica viene mappata come un numero reale.
</td>
  </tr>
  <tr>
    <td>MW300</td>
    <td>Indirizzo della word in memoria a partire dalla locazione 300.
</td>
  </tr>
  <tr>
    <td>MD400</td>
    <td>Indirizzo della doppia word in memoria a partire dalla locazione 400.
</td>
  </tr>
  <tr>
    <td>MR1900</td>
    <td>Indirizzo del reale in memoria a partire dalla locazione 1900.
</td>
  </tr>
</table>


	

### **2.9.2 Timers**

Possiamo defnire due tipologie di timer.

Timer ad attivazione:

Timer che attiva la propria uscita dopo che è trascorso il tempo di conteggio.

I timers di questo tipo vengono identificati con la nomenclatura **_TONx _**dove x è l’identificativo del timer (fino a 200).

In questo caso il conteggio è da considerarsi in millisecondi.

Possiamo definire anche dei timers ad attivazione con il conteggio in secondi: **_TONSx._**

Timer a disattivazione:

Timer che attiva la propria uscita con l’ingresso di enable e la disattiva dopo che è trascorso il tempo di conteggio, al disabilitarsi di enable

I timers di questo tipo vengono identificati con la nomenclatura **_TOFx _**dove x è l’identificativo del timer (fino a 200).

In questo caso il conteggio è da considerarsi in millisecondi.

Possiamo definire anche dei timers a disattivazione con il conteggio in secondi: **_TOFSx._**

### **2.9.3 Contatori**

* Contatori a incremento sul fronte di salita del clock **CREx** dove x è l’identificativo del contatore (fino a 200).

* Contatori a incremento sul livello alto del clock **CHLx** dove x è l’identificativo del contatore (fino a 200).

* Contatori a incremento sul fronte di discesa del clock **CFEx **dove x è l’identificativo del contatore (fino a 200).

* Contatori a incremento sul livello basso del clock **CHLx** dove x è l’identificativo del contatore (fino a 200).

* Contatori a decremento sul fronte di salita del clock **CDREx **dove x è l’identificativo del contatore (fino a 200).

* Contatori a decremento sul livello alto del clock **CDHLx **dove x è l’identificativo del contatore (fino a 200).

### **2.9.4 Impulsi**

* Impulso di salita **RPEx** dove x è l’identificativo dell’impulso (fino a 200).

* Impulso di discesa **FPEx** dove x è l’identificativo dell’impulso (fino a 200).

