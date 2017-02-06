#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <fstream>
using namespace std;
using namespace cv;

enum tObjs {ball, carR, carP, carBP, carBG, carYG, carYP};
typedef struct trackOBJ
{
    int lastX = -1;
    int lastY = -1;
    int posX;
    int posY;
    int blue;
    int green;
    int red;
    ofstream File;
}trackOBJ;
trackOBJ Objs[7];

//Declaracao das variaveis
int lowHue =76;
int highHue = 130;
int lowSat = 161;
int highSat = 255;
int lowVal = 126;
int highVal = 255;
bool c;
int trackBall, trackRed, trackPurple, trackBP, trackBG, trackYG, trackYP;
Mat frame, frame_HSV, frameThresh_red, frameThresh_yellow, frameThresh_green;
Mat frameThresh_ball, frameThresh_pink, frameThresh_purple, frameThresh_blue;
Mat imgLines;
vector < vector<Point> > contours;
vector <Vec4i> hierarchy;

//Abre os arquivos que conterao as posicoes de cada carro
void OpenFiles();
//Fecha os arquivos
void CloseFiles();
//Funcao que calcula a posicao do objeto e atualiza a sua posicao atual (objetos de cor unica)
Mat TrackUniqueObject(Mat thresholded, tObjs T);
//Mesma funcao acima porem para objetos com duas cores
Mat TrackNotUniqueObject(Mat thresholded1, Mat thresholded2, tObjs T);
//Cria as janelas a serem mostradas
void CreateWindows();
//Botoes para habilitar o tracking
void Trackbars();
//Constroi as imagens binarias
void Limiarizacao();
//Tira os ruidos e pequenos objetos das imagens binarias
void Suavizar();
//imshow's
void ShowImage();
//Funcao que determina qual objeto sofrera o tracking
Mat Tracking();
//Bola
Mat Track_Ball();
//Carro vermelho
Mat Track_car1();
//Carro roxo
Mat Track_car2();
//Carro rosa e azul
Mat Track_car3();
//Carro verde e azul
Mat Track_car4();
//Carro amarelo e rosa
Mat Track_car5();
//Carro amarelo e verde
Mat Track_car6();

int main(){
  VideoCapture video("Troca de goleiro.avi");
  trackBall = trackRed = trackPurple = trackBP = trackBG = trackYG = trackYP = 0;
  if(!video.isOpened()){
    cout << "Nao foi possivel acessar video" << endl;
    return -1;
  }
  OpenFiles();
  CreateWindows();
  Trackbars();

  Mat imgTmp, result;
  video.read(imgTmp);
  imgLines = Mat::zeros(imgTmp.size(), CV_8UC3);

  while(true){
          //Le um frame
          c = video.read(frame);
          if(!c){
            cout << "Nao foi possivel ler frame" << endl;
          }
          //TRansofrma a imagem para HSV
          cvtColor(frame, frame_HSV, CV_BGR2HSV);
          //blur(frame_HSV, frame_HSV, Size(5,5));
          Limiarizacao();

          Suavizar();
          ShowImage();

          result = Tracking();
          frame = frame + result;
          imshow("Original", frame);

          if(waitKey(30) == 1048603){
            cout << "Esc pressionado" << endl;
            break;
          }
          //Se a tecla R for precionada o video reinicia
          if(waitKey(30) == 1048690) {
            video.set(CV_CAP_PROP_POS_MSEC, 0);
            imgLines = Mat::zeros(imgTmp.size(), CV_8UC3);
            for(int i=0;i<7;i++) {
                Objs[i].lastX = -1;
                Objs[i].lastY = -1;
            }
          }
     }
     CloseFiles();
return 0;
}

Mat TrackUniqueObject(Mat thresholded, tObjs T){
        //Atraves da imagem binaria passada como argumento (thresholded) o controrno de todos os objetos e tracado da mesma armazenando-os na variavel contours
        findContours(thresholded, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

        Mat drawing = Mat::zeros(thresholded.size(), CV_8UC3);

        for (int i = 0; i < contours.size(); i++) {
            //Para cada contorno encontrado, o desenhamos na imagem drawing
            drawContours(drawing, contours, i, Scalar(Objs[T].blue, Objs[T].green, Objs[T].red), 2, 8, hierarchy, 0, Point());
          }
             if(contours.size() > 0){

                  Moments momObj = moments(contours.at(contours.size()-1), false);

                  double dm01 = momObj.m01;
                  double dm10 = momObj.m10;
                  double dArea = momObj.m00;
                  //cout << dArea << endl;

                  if (dArea > 30) {
                    Objs[T].posX = dm10/dArea;
                    Objs[T].posY = dm01/dArea;
                    Objs[T].File << "Posx: " << Objs[T].posX << " Posy: " << Objs[T].posY << endl;

                    if (Objs[T].lastX >= 0 && Objs[T].lastY >= 0 && Objs[T].posX >= 0 && Objs[T].posY >= 0)
                      line(imgLines, Point(Objs[T].posX, Objs[T].posY), Point(Objs[T].lastX, Objs[T].lastY),Scalar(Objs[T].blue, Objs[T].green, Objs[T].red), 2);
                        Objs[T].lastX = Objs[T].posX;
                        Objs[T].lastY = Objs[T].posY;
                  }
            }
          return imgLines + drawing;
}

Mat TrackNotUniqueObject(Mat thresholded1, Mat thresholded2, tObjs T){

        Mat fusao = thresholded1 + thresholded2;

        dilate(fusao, fusao,getStructuringElement(MORPH_ELLIPSE, Size(7, 7)) );
        erode(fusao, fusao,getStructuringElement(MORPH_ELLIPSE, Size(7, 7)) );

        vector <vector<Point> > contours0;

        findContours(fusao, contours0, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

        contours.resize(contours0.size());
        for (int i = 0;i < contours0.size(); i++){
                approxPolyDP(Mat(contours0[i]), contours[i], 3, true);
        }

        Mat drawing = Mat::zeros(fusao.size(), CV_8UC3);

        for (int i = 0; i < contours.size(); i++) {
            drawContours(drawing, contours, i, Scalar(Objs[T].blue, Objs[T].green, Objs[T].red), 2, 8, hierarchy, 0, Point());
        }
             if(contours.size() > 0){
                int maiorA = 0;
                int k = -1;
                for(int i = 0; i < contours.size(); i++) {
                  Moments momObj = moments(contours[i], false);

                  double dm01 = momObj.m01;
                  double dm10 = momObj.m10;
                  double dArea = momObj.m00;
                  //cout << dArea << endl;
                  if (dArea > maiorA)
                    {
                        maiorA = dArea;
                        k = i;
                    }
                }

                Moments momObj = moments(contours[k], false);
                double dm01 = momObj.m01;
                double dm10 = momObj.m10;
                double dArea = momObj.m00;
                  if (dArea > 250) {
                    Objs[T].posX = dm10/dArea;
                    Objs[T].posY = dm01/dArea;
                    Objs[T].File << "Posx: " << Objs[T].posX << " Posy: " << Objs[T].posY << endl;

                    if (Objs[T].lastX >= 0 && Objs[T].lastY >= 0 && Objs[T].posX >= 0 && Objs[T].posY >= 0)
                      line(imgLines, Point(Objs[T].posX, Objs[T].posY), Point(Objs[T].lastX, Objs[T].lastY),Scalar(Objs[T].blue, Objs[T].green, Objs[T].red), 2);
                    Objs[T].lastX = Objs[T].posX;
                    Objs[T].lastY = Objs[T].posY;
                  }
            }
          return imgLines + drawing;
}

void CreateWindows(){
    namedWindow("Control", CV_WINDOW_NORMAL);
    namedWindow("Original", CV_WINDOW_NORMAL);
  //namedWindow("HSV", CV_WINDOW_NORMAL);
  //namedWindow("RED", CV_WINDOW_NORMAL);
  //namedWindow("YELLOW", CV_WINDOW_NORMAL);
  //namedWindow("BLUE", CV_WINDOW_NORMAL);
  //namedWindow("GREEN", CV_WINDOW_NORMAL);
  //namedWindow("PINK", CV_WINDOW_NORMAL);
  //namedWindow("PURPLE", CV_WINDOW_NORMAL);
  //namedWindow("BALL", CV_WINDOW_NORMAL);
}

void Trackbars(){
    //createTrackbar("Low Hue", "Control", &lowHue, 179);
    //createTrackbar("High Hue", "Control", &highHue, 179);
    //createTrackbar("Low Sat", "Control", &lowSat, 255);
    //createTrackbar("High Sat", "Control", &highSat, 255);
    //createTrackbar("Low Val", "Control", &lowVal, 255);
    //createTrackbar("High Val", "Control", &highVal, 255);
    createTrackbar("Bola", "Control", &trackBall, 1);
    createTrackbar("Vermelho", "Control", &trackRed, 1);
    createTrackbar("Roxo", "Control", &trackPurple, 1);
    createTrackbar("Azul & Rosa", "Control", &trackBP, 1);
    createTrackbar("Azul & Verde", "Control", &trackBG, 1);
    createTrackbar("Amarelo & Verde", "Control", &trackYG, 1);
    createTrackbar("Amarelo & Rosa", "Control", &trackYP, 1);

}

void Limiarizacao(){
        inRange(frame_HSV, Scalar(170, 120, 0), Scalar(179, 255, 255), frameThresh_red);
        inRange(frame_HSV, Scalar(22, 29, 28), Scalar(38, 255, 255), frameThresh_yellow);
        inRange(frame_HSV, Scalar(39, 128, 56), Scalar(130, 255, 255), frameThresh_blue);
        inRange(frame_HSV, Scalar(73, 65, 110), Scalar(99, 124, 255), frameThresh_green);
        inRange(frame_HSV, Scalar(135, 61, 120), Scalar(169, 140, 255), frameThresh_pink);
        inRange(frame_HSV, Scalar(0, 115, 60), Scalar(35, 255, 255), frameThresh_ball);
        inRange(frame_HSV, Scalar(120, 30, 160), Scalar(150, 125, 255), frameThresh_purple);
}

void Suavizar(){
    //purple
    erode(frameThresh_purple, frameThresh_purple, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_purple, frameThresh_purple, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_purple, frameThresh_purple, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    erode(frameThresh_purple, frameThresh_purple, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    //Ball
    erode(frameThresh_ball, frameThresh_ball, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_ball, frameThresh_ball, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_ball, frameThresh_ball, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    erode(frameThresh_ball, frameThresh_ball, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    //Red
    erode(frameThresh_red, frameThresh_red, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_red, frameThresh_red, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_red, frameThresh_red, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    erode(frameThresh_red, frameThresh_red, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    //Yellow
    erode(frameThresh_yellow, frameThresh_yellow, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_yellow, frameThresh_yellow, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_yellow, frameThresh_yellow, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    erode(frameThresh_yellow, frameThresh_yellow, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    //Green
    erode(frameThresh_green, frameThresh_green, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_green, frameThresh_green, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_green, frameThresh_green, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    erode(frameThresh_green, frameThresh_green, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    //Blue
    erode(frameThresh_blue, frameThresh_blue, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_blue, frameThresh_blue, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_blue, frameThresh_blue, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    erode(frameThresh_blue, frameThresh_blue, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    //Pink
    erode(frameThresh_pink, frameThresh_pink, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_pink, frameThresh_pink, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    dilate(frameThresh_pink, frameThresh_pink, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
    erode(frameThresh_pink, frameThresh_pink, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)) );
}

void ShowImage(){
     //imshow("RED", frameThresh_red);
    //imshow("YELLOW", frameThresh_yellow);
    //imshow("BLUE", frameThresh_blue);
    //imshow("GREEN", frameThresh_green);
    //imshow("PINK", frameThresh_pink);
    //imshow("PURPLE", frameThresh_purple);
    //imshow("BALL", frameThresh_ball);
}
Mat Tracking() {
    Mat result;
    result =  Mat::zeros(frame.size(), CV_8UC3);
    if(trackBall) {
        result = result + Track_Ball();
    }
    if(trackRed) {
        result = result + Track_car1();
    }
    if(trackPurple) {
        result = result + Track_car2();
    }
    if(trackBP) {
        result = result + Track_car3();
    }
    if(trackBG) {
        result = result + Track_car4();
    }
    if(trackYP) {
        result = result + Track_car5();
    }
    if(trackYG) {
        result = result + Track_car6();
    }
    return result;
}
//Bola
Mat Track_Ball(){
    Objs[ball].blue = 0;
    Objs[ball].green = 90;
    Objs[ball].red = 255;
    Mat aux = TrackUniqueObject(frameThresh_ball, ball);
    return aux;
}
//Carro vermelho
Mat Track_car1(){
    Objs[carR].blue = 0;
    Objs[carR].green = 0;
    Objs[carR].red = 255;
    Mat aux = TrackUniqueObject(frameThresh_red, carR);
    return aux;
}
//Carro roxo
Mat Track_car2(){
    Objs[carP].blue = 153;
    Objs[carP].green = 51;
    Objs[carP].red = 102;
    Mat aux = TrackUniqueObject(frameThresh_purple, carP);
    return aux;
}

//Carro rosa e azul
Mat Track_car3(){
    Objs[carBP].blue = 255;
    Objs[carBP].green = 0;
    Objs[carBP].red = 0;
    Mat aux = TrackNotUniqueObject(frameThresh_pink, frameThresh_blue, carBP);
    return aux;
}

//Carro verde e azul
Mat Track_car4() {
    Objs[carBG].blue = 0;
    Objs[carBG].green = 255;
    Objs[carBG].red = 0;
    Mat aux = TrackNotUniqueObject(frameThresh_green, frameThresh_blue, carBG);
    return aux;
}

//Carro amarelo e rosa
Mat Track_car5() {
    Objs[carYP].blue = 204;
    Objs[carYP].green = 0;
    Objs[carYP].red = 204;
    Mat aux = TrackNotUniqueObject(frameThresh_pink, frameThresh_yellow, carYP);
    return aux;
}

//Carro amarelo e verde
Mat Track_car6() {
    Objs[carYG].blue = 0;
    Objs[carYG].green = 255;
    Objs[carYG].red = 255;
    Mat aux = TrackNotUniqueObject(frameThresh_green, frameThresh_yellow, carYG);
    return aux;
}

void OpenFiles(){
    Objs[ball].File.open("POS_Ball.txt");
    Objs[carR].File.open("POS_carR.txt");
    Objs[carP].File.open("POS_carP.txt");
    Objs[carBP].File.open("POS_carBP.txt");
    Objs[carBG].File.open("POS_carBG.txt");
    Objs[carYG].File.open("POS_carYG.txt");
    Objs[carYP].File.open("POS_carYP.txt");
}

void CloseFiles(){
    Objs[ball].File.close();
    Objs[carR].File.close();
    Objs[carP].File.close();
    Objs[carBP].File.close();
    Objs[carBG].File.close();
    Objs[carYG].File.close();
    Objs[carYP].File.close();
}
