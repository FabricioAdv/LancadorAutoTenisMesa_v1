//-Inclusão de bibliotécas-------------------------------------------

#include <Servo.h>
#include <EEPROM.h>

//-------------------------------------------------------------------

//-Valores EEPROM----------------------------------------------------

//EEPROM posição 1 - Valor salvo da posição do motor de passo (Não utilizado)
//EEPROM posição 2 - Valor salvo da posição do servo motor (Não utilizado)
//EEPROM posição 3 - Valor da maior pontuação
//EEPROM posição 4 - Valor da maior sequência de bolinhas rebatidas
//EEPROM posição 5 - Valor da percentagem de acertos

//-------------------------------------------------------------------

//-Definições--------------------------------------------------------

#define grau_pul 1.8

//-------------------------------------------------------------------

//-Pinos-------------------------------------------------------------
//-Analógico-
#define sens_laser_ldr A0


//-Digital-
#define fim_cur 0
#define av_sol 1
#define sens_laser 2
#define rc_sol 3
#define ponto 4
#define reset_ponto 7

#define dir_pass 8
#define pul_pass 9

//-------------------------------------------------------------------

//-Variáveis---------------------------------------------------------

int GRF = 0;

int grau_mp;
int grau_sm;
int esc_process = 0;
String leitura = "";
String grau = "";

int tempo_rebate = 5000;
boolean ball_rebatida = false;
boolean acerto = false;
boolean erro = false;

int ponto_atual = 0;
int ponto_acumulado = 0;
int lancamentos_acumulados = 0;
int sequencia = 0;
int acertos_atual = 0;
int acertos_atual_percent = 0;
int percent = 0;

long tempo_inicial;
long tempo_decor1;
long tempo_decor2;

long tempo_inicial_a;
long tempo_decor_aj;
int tempo_ajuda = 8000;
boolean exe_ja = false;

int pos_sm;
int pos_ant = 0;
int pos_fim = 0;

int pul_mp;
int pul_ant = 0;
int pul_fim = 0;
int pul_agr = 0;

//-------------------------------------------------------------------

//-Definições de programação-----------------------------------------

Servo servo;

//-------------------------------------------------------------------

//-Sub rotinas-------------------------------------------------------

void ctrl_motor_pass(int pul)
{
  pul_fim = pul;

  pul_agr = abs(pul_fim - pul_ant);
  
  if (pul_fim > pul_ant)
  {
    digitalWrite(dir_pass, HIGH);
    for (pul_mp = 0 ; pul_mp < pul_agr ; pul_mp++)
    {
      digitalWrite(pul_pass, HIGH);   
      delay(25);           
      digitalWrite(pul_pass, LOW);  
      delay(25);
    }
  }

  else if (pul_fim < pul_ant)
  {
    digitalWrite(dir_pass, LOW);
    for (pul_mp = 0 ; pul_mp < pul_agr ; pul_mp++)
    {
      digitalWrite(pul_pass, HIGH);
      delay(25);             
      digitalWrite(pul_pass, LOW); 
      delay(25);
    }
  }

  pul_ant = pul_fim;
}

boolean motor_pass_ang(int grau, boolean pul_inf)
{
  int grau_agr;
  int pulsos;

  Serial.println("Movimentando motor de passo...");

  if (grau && pul_inf)
  {
    pulsos = (int)(grau/grau_pul);
    
    do
    {
      ctrl_motor_pass(-1);
    }
    while (!digitalRead(fim_cur));

    ctrl_motor_pass(grau);

    EEPROM.update(1,0);
    EEPROM.update(1,grau);
  }
  else if (grau && !pul_inf)
  {
    pulsos = (int)(grau/grau_pul);

    ctrl_motor_pass(grau);
    
    EEPROM.update(1,0);
    EEPROM.update(1,grau);
  }

  Serial.println("Motor de passo na posicao\n");
  
  return 1;
}

boolean serv_motor_ang(int grau)
{
  Serial.println("Movimentando servo motor...");
  
  ctrl_vel_serv_motor(grau, 70);

  if (servo.read() == grau)
  {
    Serial.println("Servo motor na posicao\n");
         
    return 1;
    
    EEPROM.update(2,0);
    EEPROM.update(2,grau);
  }
}

void ctrl_vel_serv_motor(int ang, int tempo)
{
  pos_fim = ang;
  
  if (pos_fim > pos_ant)
  {
    for (pos_sm = pos_ant; pos_sm <= pos_fim; pos_sm++) 
    { 
      servo.write(pos_sm);         
      delay(30);                      
    }
  }
  
  
  else if (pos_fim < pos_ant)
  {
    for (pos_sm = pos_ant; pos_sm >= pos_fim; pos_sm--) 
    { 
      servo.write(pos_sm);            
      delay(30);                     
    }
  }

  pos_ant = pos_fim;
}

void mov_cil(boolean aciona)
{
  if (aciona)
  {
    digitalWrite(rc_sol, LOW);
    delay(100);
    digitalWrite(av_sol, HIGH);

    lancamentos_acumulados++;
  }
  else if (!aciona)
  {
    digitalWrite(av_sol, LOW);
    delay(100);
    digitalWrite(rc_sol, HIGH);
  }
}

void laser_acerto()
{  
  tempo_decor1 = millis() - tempo_inicial;

  int val_ldr = analogRead(sens_laser_ldr);
  val_ldr = map(val_ldr, 0, 1023, 0, 255);
  
  if (((digitalRead(sens_laser)) || val_ldr < 85) && GRF == 4 && (tempo_decor1 < tempo_rebate))
  {
    Serial.println("Bolinha rebatida");
    Serial.println("Computando pontos...");
    
    sinal_ponto();

    Serial.println("Resetando lancamentos...\n");

    delay (2000);

    acerto = true;

    GRF = 1;
    
    ball_rebatida = true;
  }

  else if (GRF == 4 && tempo_decor1 > tempo_rebate)
  {
    Serial.println("Erro do jogador ao rebater");
    Serial.println("Resetando lancamentos...\n");
    delay (2000);

    erro = true;
    
    GRF = 1;
    
    ball_rebatida = true;
  }
}

void sinal_ponto()
{
    digitalWrite(ponto, LOW);
    digitalWrite(ponto, HIGH);

    Serial.println("Pontos computados\n");
}

void exe_ini()
{
  tempo_decor_aj = millis() - tempo_inicial_a;
  
  if (exe_ja && !GRF && (tempo_decor_aj > tempo_ajuda))
  {
    menu();

    exe_ja = false;
  }
}

//-------------------------------------------------------------------

//-Comunicação serial------------------------------------------------

void serialEvent() 
{
  while (Serial.available()) 
  {
    leitura = Serial.readString();

    Serial.println("Dado recebido: " + leitura);

    if ((leitura == "Ligar") || (leitura == "ligar") || (leitura == "LIGAR"))
    {  
      leitura = "";
      
      esc_process = 1;
    }
    else if ((leitura == "Desligar") || (leitura == "desligar") || (leitura == "DESLIGAR"))
    {
      leitura = "";
      
      esc_process = 2;
    }
    else if ((leitura == "Home") || (leitura == "home") || (leitura == "HOME"))
    {
      leitura = "";
      
      esc_process = 3;
    }
    else if ((leitura == "Estatisticas") || (leitura == "estatisticas") || (leitura == "ESTATISTICAS"))
    {
      leitura = "";
      
      esc_process = 4;
    }
    else if ((leitura == "Resetar Estatisticas") || (leitura == "resetar estatisticas") || (leitura == "RESETAR ESTATISTICAS") || (leitura == "`Resetar estatisticas"))
    {
      leitura = "";
      
      esc_process = 5;
    }
    else
    {
      Serial.println("Entrada de dados incorreta");

      leitura = "";
      
      esc_process = 0;
    }
  }
}

void liga()
{  
  if (esc_process == 1)
  {
    if (!GRF)
    {
      Serial.println("Iniciando lancamentos...");
    
      GRF = 1;

      Serial.println("Lancamentos iniciados\n");
    }
    else
    {
      Serial.println("O sistema ja se encontra iniciado\n");
    }      

    exe_ja = true;

    esc_process = 0;
  }
}

void desl()
{
  if (esc_process == 2)
  {
    if (GRF)
    {
      Serial.println("Interrompendo lancamentos...");
    
      GRF = 0;

      motor_pass_ang(0, 0);
      
      serv_motor_ang(0);

      Serial.println("Lancamentos interrompidos\n");

      tempo_inicial_a = millis();
    }
    else
    {
      Serial.println("O sistema ja se encontra interrompido\n");
    }
    
    esc_process = 0;
  }
}

boolean reseta()
{
  if (esc_process == 3)
  {
    if (GRF == 0)
    {
      do
      {
        Serial.println("Enviando para a posicao de HOME...");
      
        motor_pass_ang(90, 1);
      
        serv_motor_ang(22);
      }
      while (!serv_motor_ang || !motor_pass_ang);

      if (serv_motor_ang && motor_pass_ang)
      {
        EEPROM.update(1,0);

        EEPROM.update(2,0);

        Serial.println("Posicao de HOME alcancada\n");
      }
    }
    else
    {
      Serial.println("O sistema se encontra iniciado");
      Serial.println("Interrompa-o para resetar o sistema\n");
    }
    
    esc_process = 0;
  }
}

void pontos()
{ 
  if (acerto)
  {
    ponto_atual++;
    ponto_acumulado++;

    if (ponto_atual > sequencia)
    {
      sequencia = ponto_atual;
    }

    if (ponto_atual > EEPROM.read(4))
    {
      EEPROM.update(4,ponto_atual);
    }

    if (ponto_acumulado > EEPROM.read(3))
    {
      EEPROM.update(3,ponto_acumulado);
    }
    
    acerto = false;
  }

  if (erro)
  {
    ponto_atual = 0;
    
    erro = false;
  }
  
  if (esc_process == 4)
  {
    if (!GRF)
    {
      String lanca = (String) lancamentos_acumulados;
      String seq = (String) sequencia;
      String acertos = (String) ponto_acumulado;
      String acertos_percent = (String) percent;
      
      String maior_seq = (String) EEPROM.read(4);
      String maior_pontuacao = (String) EEPROM.read(3);
      String maior_percent = (String) EEPROM.read(5);
      
      Serial.println("--Estatisticas de jogo--\n");
      
      Serial.println("Quantidade de lançamentos nesta partida: " + lanca);
      Serial.println("Maior sequencia nesta partida: " + seq);
      Serial.println("Quantidade de acertos nesta partida: " + acertos);
      Serial.println("Percentual de acertos nesta partida: " + acertos_percent + "\n");
      
      Serial.println("Maior sequencia de pontos: " + maior_seq);
      Serial.println("Maior potuacao: " + maior_pontuacao);
      Serial.println("Maior percentual de acertos: " + maior_percent + "\n");
    }
    else if (GRF)
    {
      Serial.println("O sistema se encontra iniciado");
      Serial.println("Interrompa-o para resetar as estatisticas\n");
    }

    esc_process = 0;
  }
  
  if (esc_process == 5)
  {
    if (!GRF)
    {
      Serial.println("Resentando estatisticas...");
      
      ponto_atual = 0;
      ponto_acumulado = 0;
    
      EEPROM.update(3,0);
      EEPROM.update(4,0);

      EEPROM.update(5,0);
      EEPROM.update(6,0);

      digitalWrite(reset_ponto, LOW);
      digitalWrite(reset_ponto, HIGH);

      Serial.println("Estatisticas resetadas\n");
    }
    else if (GRF)
    {
      Serial.println("O sistema se encontra iniciado");
      Serial.println("Interrompa-o para resetar o sistema\n");
    }
    
    esc_process = 0;
  }

  percent = (ponto_acumulado/lancamentos_acumulados)*100;

  if (percent > EEPROM.read(5))
  {
    EEPROM.update(5,percent);
  }
}

void menu()
{
  Serial.println("Digite \"ligar\" para iniciar os lancamentos");
  Serial.println("Digite \"desligar\" para interromper os lancamentos");
  Serial.println("Digite \"home\" para enviar os motores para a posicao inicial");
  Serial.println("Digite \"estatisticas\" para mostrar a pontuacao do jogo");
  Serial.println("Digite \"resetar estatisticas\" para resetar as pontuacoes do jogo\n");
}

//-------------------------------------------------------------------

//-Configuração de pinos---------------------------------------------

void config_pins()
{
  pinMode(fim_cur, INPUT);
  pinMode(av_sol, OUTPUT);
  pinMode(sens_laser, INPUT);
  pinMode(rc_sol, OUTPUT);
  pinMode(ponto, OUTPUT);
  pinMode(reset_ponto, OUTPUT);

  pinMode(dir_pass, OUTPUT);
  pinMode(pul_pass, OUTPUT);

  digitalWrite(av_sol, LOW);
  digitalWrite(rc_sol, LOW);
  digitalWrite(ponto, HIGH);
  digitalWrite(reset_ponto, HIGH);
}

//-------------------------------------------------------------------

//-Programação-------------------------------------------------------

void setup()
{
  config_pins();
  
  Serial.begin(9600);

  while (!Serial) 
  {
    ;
  }
  
  servo.attach(12);

  Serial.println("Iniciando...");
  
  delay(2000);

  Serial.println("Iniciado\n");

  menu();
}

void loop() 
{  
  exe_ini();
  
  liga(); 
  desl();
  reseta();
  pontos();
  
  if (GRF == 1)
  {
    mov_cil(0);    

    ball_rebatida = false;
    
    GRF = 2;
  }
  
  else if (GRF == 2)
  {
    grau_mp = random (0, 15);

    grau = (String) grau_mp;

    Serial.println("Angulo randomico do motor de passo: " + grau);

    grau = "";

    motor_pass_ang(grau_mp, 0);

    if (motor_pass_ang)
    {      
      GRF = 3;
    }
  }

  else if (GRF == 3)
  {
    grau_sm = random (50, 60);

    grau = (String) grau_sm;

    Serial.println("Angulo randomico do servo motor: " + grau);

    grau = "";

    serv_motor_ang(grau_sm);
        
    if (serv_motor_ang)
    {   
      GRF = 4;
    }
  }

  else if (GRF == 4)
  {
    if (serv_motor_ang && motor_pass_ang)
    {
      Serial.println("Lancando bolinha...");
      
      delay (1000);
      
      mov_cil(1);

      Serial.println("Bolinha lancada\n");

      Serial.println("Aguardando rebate...\n");

      tempo_inicial = millis();

      do
      {
        laser_acerto();
      }
      while (!ball_rebatida);
    }
  }
}

//-------------------------------------------------------------------
