#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "VsPlugin.h"
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include "vsroomlink.h"

#define SAFE_STRING_CPY(destination, source, limit){\
    size_t sizeToCopy = (limit > 0) ? (limit -1) : 0;\
    strncpy(destination, source, sizeToCopy);\
    }

#define MAX_STREAMS_RENDER  4
#define STREAM_HEIGHT   320
#define STREAM_WIDTH    180

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ShowAfterStop();
    vsPlugin = VsPluginPtr(new VsPlugin(this));
    connect(ui->checkBoxDynamicRawFrame, SIGNAL(stateChanged(int)), this, SLOT(DynamicOrStatic(int)));
    m_bDynamicRawFrames = false;
    m_bTestPreviewOn = false;

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButtonStart_clicked()
{
    if(!vsPlugin->VsStart())
    {
        QMessageBox msgBox;
        msgBox.setText("Error");
        msgBox.setInformativeText("VidyoClientStart");
        msgBox.exec();
        return;

    }
    ShowAfterStart();
}


void MainWindow::ShowAfterStart()
{
    ui->pushButtonStart->setEnabled(false);
    ui->pushButton_Stop->setEnabled(true);
    ui->pushButtonRoomLink->setEnabled(true);
    ui->pushButtonEndCall->setEnabled(true);
    ui->pushButtonRefresh->setEnabled(true);
    ui->checkBoxDynamicRawFrame->setEnabled(true);
    ui->pushButtonPreview->setEnabled(true);
}

void MainWindow::ShowAfterStop()
{
    ui->pushButtonStart->setEnabled(true);
    ui->pushButton_Stop->setEnabled(false);
    ui->pushButtonRoomLink->setEnabled(false);
    ui->pushButtonEndCall->setEnabled(false);
    ui->pushButtonRefresh->setEnabled(false);
    ui->checkBoxDynamicRawFrame->setEnabled(false);
    ui->pushButtonPreview->setEnabled(false);
}


void MainWindow::on_pushButton_Stop_clicked()
{
    if(!vsPlugin->VsStop())
    {
        QMessageBox msgBox;
        msgBox.setText("Error");
        msgBox.setInformativeText("VidyoClientStop");
        msgBox.exec();
        return;

    }
    ShowAfterStop();
}

void MainWindow::on_pushButtonRoomLink_clicked()
{
    QScopedPointer<VsRoomLink> roomLink(new VsRoomLink(this));
    roomLink->exec();

    if(roomLink->result.isEmpty())
        return;
    if(!vsPlugin->VsRoomLink(roomLink->result))
    {
        QMessageBox msgBox;
        msgBox.setText("Error");
        msgBox.setInformativeText("Room link");
        msgBox.exec();
        return;
    }


}

void MainWindow::on_pushButtonEndCall_clicked()
{
    vsPlugin->VsCallEnd();
}

void MainWindow::InCall()
{
    ui->label_CallStatus->setText("In call");
    if(m_bTestPreviewOn)
    {
        //if test preview is on
        //since we are already in the call, let us turn it off
        on_pushButtonPreview_clicked();
        //since we have preview on, how about we also turn on //dynamic raw frame automatically
        QTimer::singleShot(1000, this, SLOT(on_pushButtonRefresh_clicked()));

    }
}

void MainWindow::NotInCall()
{
    ui->label_CallStatus->setText("not in In call");
}

void MainWindow::VideoMute(bool value)
{
    if(value)
        ui->label_MuteStatusVideo->setText("video muted");
    else
        ui->label_MuteStatusVideo->setText("video unmuted");
}

void MainWindow::MicMute(bool value)
{
    if(value)
        ui->label_MuteStatusMic->setText("mic muted");
    else
        ui->label_MuteStatusMic->setText("mic unmuted");
}


void MainWindow::RefreshStatic()
{
    QMapIterator<QString, int> iter(uriToWidget);

    while(iter.hasNext())
    {
        iter.next();
        QString uri(iter.key());
        VidyoClientInEventStopWatchVideoSource stopWatch = {0};
        SAFE_STRING_CPY((char *)stopWatch.source.participantURI, uri.toStdString().c_str(), sizeof(stopWatch.source.participantURI));
        stopWatch.source.mediaType = VIDYO_CLIENT_MEDIA_CONTROL_TYPE_VIDEO;
        VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_STOP_WATCH_VIDEO_SOURCE, &stopWatch, sizeof(stopWatch));
    }
    uriToWidget.clear();
    VidyoClientRequestParticipants reqParticipants = {};
    VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_PARTICIPANTS, &reqParticipants, sizeof(reqParticipants));
    int numOfParticipants = reqParticipants.numberParticipants;
    if(numOfParticipants > MAX_STREAMS_RENDER)
        numOfParticipants = MAX_STREAMS_RENDER;

    for(unsigned index = 0; index < numOfParticipants; ++index)
    {
        VidyoClientInEventStartWatchVideoSource watchSource = {};
        strcpy(watchSource.source.participantURI, reqParticipants.URI[index]);
        watchSource.source.mediaType = VIDYO_CLIENT_MEDIA_CONTROL_TYPE_VIDEO;
        watchSource.source.sourceId = 0;
        watchSource.frameRate = 15;
        watchSource.height = STREAM_HEIGHT;
        watchSource.width = STREAM_WIDTH;
        watchSource.minFrameInterval = 0;
        watchSource.watchData = NULL;
        VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_START_WATCH_VIDEO_SOURCE, &watchSource, sizeof(watchSource));
        QString uri = QString::fromUtf8((const char *)(reqParticipants.URI[index]));
        uriToWidget.insert(uri,index);
    }
}


void MainWindow::RefreshDynamic()
{
    QMapIterator<QString, int> iter(uriToWidget);

    while(iter.hasNext())
    {
        iter.next();
        QString uri(iter.key());
        VidyoClientInEventStopWatchVideoSource stopWatch = {0};
        SAFE_STRING_CPY((char *)stopWatch.source.participantURI, uri.toAscii().data(), sizeof(stopWatch.source.participantURI));
        stopWatch.source.mediaType = VIDYO_CLIENT_MEDIA_CONTROL_TYPE_VIDEO;
        VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_STOP_WATCH_VIDEO_SOURCE, &stopWatch, sizeof(stopWatch));
    }
    uriToWidget.clear();

    VidyoClientInEventSetDynamicWatchVideoSource watchSource = {};

    watchSource.numParticipants = MAX_STREAMS_RENDER;
    for(unsigned index = 0; index < watchSource.numParticipants; ++index)
    {
        watchSource.dynamicViews[index].maxFrameRate = 15;
        watchSource.dynamicViews[index].height = STREAM_HEIGHT;
        watchSource.dynamicViews[index].width = STREAM_WIDTH;
    }
    VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_SET_DYNAMIC_WATCH_VIDEO_SOURCE, &watchSource, sizeof(watchSource));
}

void MainWindow::on_pushButtonRefresh_clicked()
{
   if(m_bDynamicRawFrames)
      RefreshDynamic();
   else
      RefreshStatic();
}


void ConvertFrameToOpenCV(void* frameBytes, QImage& m_RGB, int width, int height){
    //if(!m_RGB)  m_RGB = QImage(width, height, QImage::Format_RGB888);


    unsigned char* pData = (unsigned char *) frameBytes;


    for(int i = 0, j=0; i < width * height * 3; i+=6, j+=4)
    {

        unsigned char u = pData[j];
        unsigned char y = pData[j+1];
        unsigned char v = pData[j+2];

        //fprintf(stderr, "%d\n", v);


        m_RGB.bits()[i+2] = 1.0*y + 8 + 1.402*(v-128);               // r
        m_RGB.bits()[i+1] = 1.0*y - 0.34413*(u-128) - 0.71414*(v-128);   // g
        m_RGB.bits()[i] = 1.0*y + 1.772*(u-128) + 0;                            // b

        y = pData[j+3];
        m_RGB.bits()[i+5] = 1.0*y + 8 + 1.402*(v-128);               // r
        m_RGB.bits()[i+4] = 1.0*y - 0.34413*(u-128) - 0.71414*(v-128);   // g
        m_RGB.bits()[i+3] = 1.0*y + 1.772*(u-128) + 0;
    }


}


void MainWindow::AddToMapIfRequired(QString& uri)
{
    if(m_bTestPreviewOn)
    {
        if(uriToWidget.contains(uri))
        {
            //this participant already exists
            return;
        }
        uriToWidget.insert(uri,0);
        return;
    }
    if(!m_bDynamicRawFrames)
    {
        //we only add at run time for dynamic frames
        return;
    }
    if(uriToWidget.contains(uri))
    {
        //this participant already exists
        return;
    }
    if(uriToWidget.size() >= MAX_STREAMS_RENDER)
    {
        //there is no more place to add
        return;
    }
    uriToWidget.insert(uri,uriToWidget.size());

}

QWidget* MainWindow::GetWidgetForUri(QString& uri)
{
    AddToMapIfRequired(uri);
    if(!uriToWidget.contains(uri))
    {
        return NULL;
    }
    int index = uriToWidget[uri];
    QWidget* answer = NULL;
    switch(index)
    {
    case 0:
        answer = (QWidget*)ui->openGLWidget1;
        break;
    case 1:
        answer = (QWidget*)ui->openGLWidget2;
        break;
    case 2:
        answer = (QWidget*)ui->openGLWidget3;
        break;
    case 3:
        answer = (QWidget*)ui->openGLWidget4;
        break;
    }
    return answer;

}


void MainWindow::OnReceivedFrame(void* videoFrameVoid)
{
    VidyoClientOutEventVideoFrameReceived *videoFrame = (VidyoClientOutEventVideoFrameReceived*)videoFrameVoid;

    VidyoClientMediaControlType format = videoFrame->mediaType;
    if(format != VIDYO_CLIENT_MEDIA_CONTROL_TYPE_VIDEO)
        return;

    VidyoClientInEventVideoFrame *pFrame = &videoFrame->frame;
    if (pFrame == NULL || pFrame->size <= 0)
        return;
    QString uri = QString::fromUtf8((const char *)(videoFrame->participantURI));
    QWidget* widget = GetWidgetForUri(uri);
    if(!widget)
    {
        return;
    }

    /*int imageWidth = pFrame->width;
    int imageHeight = pFrame->height;
     int bytesPerPixel = 4; // 4 for RGBA, 3 for RGB
    QImage image(pFrame->data, imageWidth, imageHeight, imageWidth * bytesPerPixel, QImage::Format_ARGB32);
    ConvertFrameToOpenCV(pFrame->data, image, imageHeight, imageHeight);*/

    int imageWidth = pFrame->width;
    int imageHeight = pFrame->height;
    QImage image((uchar*)pFrame->data, imageWidth, imageHeight, imageWidth * 4, QImage::Format_RGB32);
    MyGlCanvas *openGLWidget = (MyGlCanvas *)widget;
    openGLWidget->setImage(image);
    openGLWidget->update();
}

void MainWindow::DynamicOrStatic(int state)
{

    m_bDynamicRawFrames = (state == Qt::Checked);

}

void MainWindow::on_pushButtonPreview_clicked()
{
   VidyoClientInEventPrecallTestCamera previewCamera = {};
   if(!m_bTestPreviewOn)
   {
       previewCamera.action = VIDYO_CLIENT_DEVICE_TEST_START;
   }
   else
   {
        previewCamera.action = VIDYO_CLIENT_DEVICE_TEST_STOP;
   }
   VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_PRECALL_TEST_CAMERA, &previewCamera, sizeof(previewCamera));
   m_bTestPreviewOn = !m_bTestPreviewOn;
}
