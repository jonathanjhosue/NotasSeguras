/***************************************************************************
 *   Copyright (C) 2007 Lukas Kropatschek <lukas.krop@kdemail.net>         *
 *   Copyright (C) 2008 Sebastian Kügler <sebas@kde.org>                   *
 *   Copyright (C) 2008 Davide Bettio <davide.bettio@kdemail.net>          *
 *   Copyright (C) 2012 Jonathan Sánchez U <jonathanjhosue@gmail.com>      *
 *								       *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "notasseguras.h"

#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <QtGui/QGraphicsGridLayout>
#include <QtGui/QGraphicsLinearLayout>
#include <QtGui/QGraphicsTextItem>
#include <QtGui/QScrollBar>
#include <QtGui/QToolButton>
#include <QtGui/QMenu>
#include <qpainter.h>
#include <QParallelAnimationGroup>

#include <KConfigDialog>
#include <KConfigGroup>
#include <KGlobalSettings>
#include <KFileDialog>
#include <KMessageBox>
#include <KIcon>
#include <KPushButton>
#include <KStandardAction>
#include <KAction>

#include <Plasma/Animator>
#include <Plasma/Animation>
#include <Plasma/PushButton>
#include <Plasma/Theme>
#include <Plasma/LineEdit>
#include <klineedit.h>

#include "textedit.h"
#include "simplecrypt.h"

class TopWidget : public QGraphicsWidget
{
public:
    TopWidget(QGraphicsWidget *parent)
        : QGraphicsWidget(parent),
          m_notesTheme(new Plasma::Svg(this)),
          m_color("white-notes")
    {
        m_notesTheme->setImagePath("widgets/fondo");
        m_notesTheme->setContainsMultipleImages(false);
	
    }

    void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        Q_UNUSED(option)
        Q_UNUSED(widget)

        m_notesTheme->resize(geometry().size());
	
	  	
	
        m_notesTheme->paint(p, contentsRect(), m_color);
	
    }

    bool hasColor(const QString &color) const
    {
        return m_notesTheme->hasElement(color + "-notes");
    }

    QString color() const
    {
        return QString(m_color).remove("-notes");
    }

    void setColor(QString color)
    {
        color.remove("-notes");
        if (hasColor(color)) {
            m_color = color + "-notes";
        }
    }

private:
    Plasma::Svg *m_notesTheme;
    QString m_color;
};



NotasSeguras::NotasSeguras(QObject *parent, const QVariantList &args)
    : Plasma::PopupApplet(parent, args),
      m_wheelFontAdjustment(0),
      m_layout(0),
      m_textEdit(0),
      m_lineEdit(0)
{
    setStatus(Plasma::AcceptingInputStatus);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);
    setHasConfigurationInterface(true);
    setAcceptDrops(true);
    setAcceptsHoverEvents(true);
    setBackgroundHints(Plasma::Applet::NoBackground);
    m_saveTimer.setSingleShot(true);
    connect(&m_saveTimer, SIGNAL(timeout()), this, SLOT(saveNote()));
    resize(256, 256);

    m_topWidget = new TopWidget(this);
    m_layout = new QGraphicsLinearLayout(Qt::Vertical);
    m_topWidget->setLayout(m_layout);

    m_textEdit = new Plasma::TextEdit(m_topWidget);
    m_textEdit->setMinimumSize(QSize(60, 60)); //Ensure a minimum size (height) for the textEdit
    
    m_lineEdit = new Plasma::LineEdit(m_topWidget);
    m_lineEdit->setOpacity(0.75);
    m_lineEdit->nativeWidget()->setEchoMode(QLineEdit::Password);

    KTextEdit *w = m_textEdit->nativeWidget();
    m_noteEditor = new NotesTextEdit(this);
    m_noteEditor->setFrameShape(QFrame::NoFrame);
    m_noteEditor->viewport()->setAutoFillBackground(false);
    m_noteEditor->setWindowFlags(m_noteEditor->windowFlags() | Qt::BypassGraphicsProxyWidget);
    if (m_noteEditor->verticalScrollBar() && w->verticalScrollBar()) {
        m_noteEditor->verticalScrollBar()->setStyle(w->verticalScrollBar()->style());
    }
    //FIXME: we need a way to just add actions without changing the noteEditor widget under its feet
    m_textEdit->setNativeWidget(m_noteEditor);
    m_textEdit->setEnabled(false);
    // scrollwheel + ctrl changes font size
    m_layout->setSpacing(2); //We need a bit of spacing between the edit and the buttons
    m_layout->addItem(m_textEdit);
    m_layout->addItem(m_lineEdit);
    m_textColor= QColor(0,0,0);
    /*passwordLayout=new QGraphicsLinearLayout(Qt::Vertical);
    
    m_passw1LineEdit=new Plasma::LineEdit(m_topWidget);
    m_passw1LineEdit->nativeWidget()->setPlaceholderText(i18n("new password"));
    m_passw1LineEdit->nativeWidget()->setEchoMode(QLineEdit::Password);
    m_passw2LineEdit=new Plasma::LineEdit(m_topWidget);  
    m_passw2LineEdit->nativeWidget()->setPlaceholderText(i18n("repeat password"));
    m_passw2LineEdit->nativeWidget()->setEchoMode(QLineEdit::Password);
    
    m_passwPushButton=new Plasma::PushButton(m_topWidget);  
    m_passwPushButton->setText(i18n("&OK"));
    m_passwPushButton->setIcon(KIcon("dialog-ok"));
    m_passwPushButton->setEnabled(false);
    m_passwPushButton->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
	    
    passwordLayout->addItem(m_passw1LineEdit);
    passwordLayout->addItem(m_passw2LineEdit);
    passwordLayout->addItem(m_passwPushButton);
    
    m_layout->addItem(passwordLayout);*/
   
    
    if (args.count() > 0) {
        m_filePath=args.at(0).toString();
	
    }
    numberkey=Q_UINT64_C(0x0c2ad4a4acb9f023);
    m_crypto=SimpleCrypt(numberkey);

    createTextFormatingWidgets();
    
    setPopupIcon("notasseguras");
    setGraphicsWidget(m_topWidget);
    
}

/*
void NotasSeguras::openNote(const QString &path){
  KUrl url = KUrl(path);
  QFile f(url.path());
KMessageBox::error(0,path);

  if (f.open(QIODevice::ReadOnly)) {
      QTextStream t(&f);
      //m_noteEditor->setTextOrHtml(t.readAll());
      m_text=t.readAll();
      //openNote();
      QTimer::singleShot(1000, this, SLOT(saveNote()));
      f.close();
      KMessageBox::error(0,m_text);
  }
}*/


NotasSeguras::~NotasSeguras()
{
    saveNote();
    delete m_textEdit;
    delete m_lineEdit;
   /* delete m_passw1LineEdit;
    delete m_passw2LineEdit;*/
    delete m_colorMenu;
    delete m_formatMenu;
    delete widget;
}

void NotasSeguras::init()
{
    m_colorMenu = new QMenu(i18n("Notes Color"));
    connect(m_colorMenu, SIGNAL(triggered(QAction*)), this, SLOT(changeColor(QAction*)));
    addColor("white", i18n("White"));
    addColor("black", i18n("Black"));
    addColor("red", i18n("Red"));
    addColor("orange", i18n("Orange"));
    addColor("yellow", i18n("Yellow"));
    addColor("green", i18n("Green"));
    addColor("blue", i18n("Blue"));
    addColor("pink", i18n("Pink"));
    addColor("translucent", i18n("Translucent"));

    m_autoFont = false;

    configChanged();   
    //openNote(m_filePath);  
  
    connect(m_noteEditor, SIGNAL(error(QString)), this, SLOT(showError(QString)));
    connect(m_noteEditor, SIGNAL(scrolledUp()), this, SLOT(increaseFontSize()));
    connect(m_noteEditor, SIGNAL(scrolledDown()), this, SLOT(decreaseFontSize()));
    connect(m_noteEditor, SIGNAL(cursorMoved()), this, SLOT(delayedSaveNote()));
    connect(m_noteEditor, SIGNAL(cursorMoved()), this, SLOT(lineChanged()));
    connect(m_textEdit, SIGNAL(textChanged(QString)), this, SLOT(delayedSaveNote()));
    connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(passwordChanged(QString)));
    
   /* connect(m_passw1LineEdit, SIGNAL(textChanged(QString)), this, SLOT(createPasswordChange()));
    connect(m_passw2LineEdit, SIGNAL(textChanged(QString)), this, SLOT(createPasswordChange()));
	
    connect(m_passwPushButton, SIGNAL(clicked()), this, SLOT(saveCreatePassword()));*/
    
   
    /*KMessageBox *uno= new KMessageBox;
    
   //Q_UINT64_C(0x0c2ad4a4acb9f023);
    uno->about(new QWidget(),"Mensaje de prueba  Random:"+QString(qrand())+" random2"+QString(qrand()),"prueba");*/
  
    /*KMessageBox::error(0,"WW text:"+m_textEncrypted+"\n t:"+m_text+"\n p:"+m_password+"");*/
    //configChanged();
}

void NotasSeguras::configChanged()
{
    KConfigGroup cg = config();
    m_topWidget->setColor(cg.readEntry("color", "white"));
    // color must be before setPlainText("foo")
    m_useThemeColor = cg.readEntry("useThemeColor", false);
    m_useNoColor = cg.readEntry("useNoColor", true);
    if (m_useThemeColor) {
        m_textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::ButtonTextColor);
        connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeChanged()));
    } else {
        m_textColor = cg.readEntry("textColor", m_textColor);
    }
    m_textBackgroundColor = cg.readEntry("textBackgroundColor", QColor(Qt::transparent));

    m_font = cg.readEntry("font", KGlobalSettings::generalFont());

    m_customFontSize = cg.readEntry("customFontSize", m_font.pointSize());
    m_autoFont = cg.readEntry("autoFont", false);
    m_autoFontPercent = cg.readEntry("autoFontPercent", 4);

    /*m_checkSpelling = cg.readEntry("checkSpelling", false);
    m_noteEditor->setCheckSpellingEnabled(m_checkSpelling);*/
    
    m_password= cg.readEntry("keyPassword","");
    
    //llamar al decrypted
    numberkey=cg.readEntry("numberKey",numberkey);    
    m_crypto.setKey(numberkey);
    
    
    m_textEncrypted= cg.readEntry("autoSaveHtml", QString());
    
    if(!m_textEncrypted.isEmpty()){
      m_text=m_crypto.decryptToString(m_textEncrypted);
    }      
    
    if(!m_password.isEmpty()){
       m_password=m_crypto.decryptToString(m_password);
    }else{
      
    }
    
    
    
    /*if (text.isEmpty()) {
        // see if the old, plain text version is still there?
        text = cg.readEntry("autoSave", QString());
        if (!text.isEmpty()) {
            m_noteEditor->setText(text);
            cg.deleteEntry("autoSave");
            saveNote();
        }
    } else {
        m_noteEditor->setHtml(text);
    }*/
    
    
    
     //KMessageBox::error(0,"EE text:"+m_textEncrypted+"\n t:"+m_text+"\n p:"+m_password+"");
    
   
    
    if(m_password.isEmpty()){
      /*m_noteEditor->setVisible(false);
      widget->setVisible(false);
      m_lineEdit->setVisible(false);
      */
      //m_password=m_crypto.encryptToString(QString(""));
      /* KMessageBox *un= new KMessageBox;    
      un->about(new QWidget(),m_password,"prueba");
      
      QString uno=m_crypto.encryptToString(QString("1"));
      QString dos=m_crypto.encryptToString(QString("1"));*/
      setConfigurationRequired(true,i18n("First set a password "));
      //showMessage(KIcon("object-unlocked"),i18n("First set a password"),Plasma::ButtonOk);             
     
      
      /*
      passLayout->addItem(pass1);
      passLayout->addItem(pass2);
      */
      
    }
    else{
      setConfigurationRequired(false,i18n("Password exist"));
       //m_password=m_crypto.decryptToString(m_password);
    }
    
    
    
    

    //Set the font family and color, it may have changed from the outside
    QTextCursor oldCursor = m_noteEditor->textCursor();
    m_noteEditor->selectAll();
    m_textEdit->setFont(m_font);
    m_noteEditor->setTextColor(m_textColor);
    m_noteEditor->setTextCursor(oldCursor);
    

    int scrollValue = cg.readEntry("scrollValue").toInt();
    if (scrollValue) {
        m_noteEditor->verticalScrollBar()->setValue(scrollValue);
    }
    
    /*KMessageBox *uno= new KMessageBox;
    uno->about(new QWidget(),"Mensaje de pruebaddd ","prueba");*/
    //KLineEdit *password = new KLineEdit;
   //Plasma::LineEdit *password=new Plasma::LineEdit(m_topWidget);

    
    //password->nativeWidget()->setEchoMode(QLineEdit::Password);
    //m_layout->addItem(password);
    
    //KMessageBox::error(0,"configChanged:"+m_password);
    
    // KMessageBox::error(0,"EE text:"+m_textEncrypted+"\n p:"+m_password+"");
    /*if(!m_text.isEmpty()){
       KMessageBox::error(0,"EE1 text:"+m_text+"");
    }
    if(!m_password.isEmpty()){
       KMessageBox::error(0,"EE2 p:"+m_password+"");
      
    }*/
    

    updateTextGeometry();
    
    // make sure changes to the background colour take effect immediately
    update();
    
   
    

}

void NotasSeguras::showError(const QString &message)
{
    showMessage(KIcon("dialog-error"), message, Plasma::ButtonOk);
}

/**
* this function is called when you change the line you are editing
* to change the background color
*/
void NotasSeguras::lineChanged()
{
    //Re-set the formatting if previous text was deleted
    QTextCursor textCursor = m_noteEditor->textCursor();
    if (textCursor.atStart()) {
        QTextCharFormat fmt;
        fmt.setForeground(QBrush(m_textColor));
        fmt.setFont(m_font);
        m_noteEditor->setCurrentCharFormat(fmt);
    }

    if (m_useNoColor) {
        return;
    }

    QTextEdit::ExtraSelection textxtra;
    textxtra.cursor = m_noteEditor->textCursor();
    textxtra.cursor.movePosition(QTextCursor::StartOfLine);
    textxtra.cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
    textxtra.format.setBackground(m_textBackgroundColor);

    QList<QTextEdit::ExtraSelection> extras;
    extras << textxtra;
    m_noteEditor->setExtraSelections( extras );

    update();
}


/**
* this function is called when you change the password 
* for show text
*/
void NotasSeguras::passwordChanged(QString contrasena)
{
   //contrasena=m_crypto.encryptToString(contrasena);  
   if(!contrasena.isEmpty() && contrasena==m_password){
     //openfNote(m_filePath);
     //QString decrypted ="";
    //if(!m_textEncrypted.isEmpty()){
	//decrypted = m_textEncrypted;	
	m_noteEditor->setTextOrHtml(m_text);
    //}
  
 //QString result = crypto.encryptToString(testString);
  
   
   //m_noteEditor->setPlainText(result);
   //NotasSeguras::openNote(decrypted);
   /*
    KMessageBox *uno= new KMessageBox;
    uno->about(new QWidget(),"texto encriptado"+m_textEncrypted+ "  m_passsword"+m_password+"  password"+m_lineEdit->text(),"prueba");
    */
    m_lineEdit->nativeWidget()->setStyleSheet("color:rgb(25,25,255);");
    m_textEdit->nativeWidget()->setEnabled(true);
  }else{
    m_lineEdit->setStyleSheet("color:rgb(0,0,0);");
      m_textEdit->setEnabled(false);
     m_noteEditor->setTextOrHtml("");
  }
  
  
}

void NotasSeguras::constraintsEvent(Plasma::Constraints constraints)
{
    //XXX why does everything break so horribly if I remove this line?
    setBackgroundHints(Plasma::Applet::NoBackground);
    if (constraints & Plasma::SizeConstraint) {
        updateTextGeometry();
    }

    if (constraints & Plasma::FormFactorConstraint) {
        if (formFactor() == Plasma::Horizontal || formFactor() == Plasma::Vertical) {
            setAspectRatioMode(Plasma::ConstrainedSquare);
        } else {
            setAspectRatioMode(Plasma::IgnoreAspectRatio);
        }
    }
}

void NotasSeguras::updateTextGeometry()
{
    if (m_layout) {
        //FIXME: this needs to come from the svg
        const qreal xpad = geometry().width() / 15;
        const qreal ypad = geometry().height() / 15;
        m_layout->setContentsMargins(xpad, ypad, xpad, ypad);
        m_font.setPointSize(fontSize());

        QTextCursor oldTextCursor = m_noteEditor->textCursor();
        m_noteEditor->selectAll();
        m_noteEditor->setFontPointSize(m_font.pointSize());
        m_noteEditor->setTextCursor(oldTextCursor);

        if (m_autoFont) {
            lineChanged();
        }
    }
}

int NotasSeguras::fontSize()
{
    if (m_autoFont) {
        int autosize = qRound(((geometry().width() + geometry().height())/2)*m_autoFontPercent/100)  + m_wheelFontAdjustment;
        return qMax(KGlobalSettings::smallestReadableFont().pointSize(), autosize);
    } else {
        return m_customFontSize + m_wheelFontAdjustment;
    }
}

void NotasSeguras::increaseFontSize()
{
    m_wheelFontAdjustment++;
    updateTextGeometry();
}

void NotasSeguras::decreaseFontSize()
{

    if (KGlobalSettings::smallestReadableFont().pointSize() < fontSize()) {
        m_wheelFontAdjustment--;
        updateTextGeometry();
    }
}

void NotasSeguras::delayedSaveNote()
{
    m_saveTimer.start(3500);
}

void NotasSeguras::saveNote()
{
    KConfigGroup cg = config();
     
    //QString texto=
    // KMessageBox::error(0,"SaveNote text:"+m_text+"\n");
    if(!m_password.isEmpty() && m_lineEdit->text()==m_password){  
     
      //QString texto=m_textEncrypted;
      m_textEncrypted=m_crypto.encryptToString(m_noteEditor->textOrHtml());
      m_text=m_noteEditor->textOrHtml();
      //=result;	
      //decrypted = crypto.decryptToString(result);
      saveState(cg);
      
      
      emit configNeedsSaving();
    }
      //m_textEncrypted=
      
}


void NotasSeguras::createPasswordChange()
{
    /*if(!m_passw1LineEdit->text().isEmpty() && (m_passw1LineEdit->text()==m_passw2LineEdit->text())){
      m_passwPushButton->setEnabled(true);
      m_passwPushButton->setIcon(KIcon("dialog-error"));
      
    }   else{
      m_passwPushButton->setEnabled(false);
    }*/
    
}


void NotasSeguras::saveCreatePassword()
{
   /* KConfigGroup cg = config();    
    m_password=m_crypto.encryptToString(m_passw1LineEdit->text()); 
    qsrand(QTime::currentTime().msec());
    numberkey=qrand();
    cg.writeEntry("numberKey", numberkey);
    cg.writeEntry("keyPassword", m_password);
    emit configNeedsSaving();*/
}



void NotasSeguras::saveState(KConfigGroup &cg) const
{  
 
 //QString password_plain=m_crypto.decryptToString(m_password);
  if(!m_password.isEmpty() && m_lineEdit->text()==m_password){
     
     //m_noteEditor->setTextOrHtml(texto);
     // KMessageBox::error(0,"SaveState text:"+m_textEncrypted+"\n "+m_text);
     cg.writeEntry("autoSaveHtml", m_textEncrypted);
      //cg.writeEntry("autoSaveHtml", "");

      cg.writeEntry("scrollValue", QVariant(m_noteEditor->verticalScrollBar()->value()));
  }
  
    //cg.writeEntry("autoSaveHtml", m_lineEditor->toHtml());
   
}

bool NotasSeguras::validPassword(){
  bool valid=false;
  if(!m_password.isEmpty() && !m_lineEdit->text().isEmpty()){
    const QString password_plain=m_password;
    if(m_lineEdit->text()==password_plain){
      valid=true;
    }
  }
  
  return valid;
}

void NotasSeguras::themeChanged()
{
    if (m_useThemeColor) {
        m_textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::ButtonTextColor);
        update();
    }
}

void NotasSeguras::addColor(const QString &id, const QString &colorName)
{
    if (m_topWidget->hasColor(id)) {
        QAction *tmpAction = m_colorMenu->addAction(colorName);
        tmpAction->setProperty("color", id);
    }
}

void NotasSeguras::changeColor(QAction *action)
{
    if (!action || action->property("color").type() != QVariant::String) {
        return;
    }

    m_topWidget->setColor(action->property("color").toString());
    KConfigGroup cg = config();
    cg.writeEntry("color", m_topWidget->color());
    emit configNeedsSaving();
    update();
}

QList<QAction *> NotasSeguras::contextualActions()
{
    QList<QAction *> actions;
    actions.append(m_colorMenu->menuAction());
    actions.append(m_formatMenu->menuAction());
    return actions;
}

void NotasSeguras::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *widget = new QWidget(parent);
    ui.setupUi(widget);
    parent->addPage(widget, i18n("General"), "notasseguras");
    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));

    QButtonGroup *fontSizeGroup = new QButtonGroup(widget);
    fontSizeGroup->addButton(ui.autoFont);
    fontSizeGroup->addButton(ui.customFont);
   

    ui.textColorButton->setColor(m_textColor);
    ui.textBackgroundColorButton->setColor(m_textBackgroundColor);
    ui.fontStyleComboBox->setCurrentFont(m_font);
    ui.fontBoldCheckBox->setChecked(m_font.bold());
    ui.fontItalicCheckBox->setChecked(m_font.italic());
    ui.autoFont->setChecked(m_autoFont);
    ui.autoFontPercent->setEnabled(m_autoFont);
    ui.customFont->setChecked(!m_autoFont);
    ui.customFontSizeSpinBox->setEnabled(!m_autoFont);
    ui.autoFontPercent->setValue(m_autoFontPercent);
    ui.customFontSizeSpinBox->setValue(m_customFontSize);

    QButtonGroup *FontColorGroup = new QButtonGroup(widget);
    FontColorGroup->addButton(ui.useThemeColor);
    FontColorGroup->addButton(ui.useCustomColor);
    ui.useThemeColor->setChecked(m_useThemeColor);
    ui.useCustomColor->setChecked(!m_useThemeColor);

    QButtonGroup *BackgroundColorGroup = new QButtonGroup(widget);
    BackgroundColorGroup->addButton(ui.useNoColor);
    BackgroundColorGroup->addButton(ui.useCustomBackgroundColor);
    ui.useNoColor->setChecked(m_useNoColor);
    ui.useCustomBackgroundColor->setChecked(!m_useNoColor);

   /* ui.checkSpelling->setChecked(m_checkSpelling);*/

    QList<QAction *> colorActions = m_colorMenu->actions();
    const QString currentColor = m_topWidget->color();
    for (int i = 0; i < colorActions.size(); i++){
        QString text = colorActions.at(i)->text().remove('&');
        if (!text.isEmpty()){
            ui.notesColorComboBox->insertItem(i, text);
            if (colorActions.at(i)->property("color").toString() == currentColor) {
                ui.notesColorComboBox->setCurrentIndex(i);
            }
        }
    }
    
    //qDebug("m_password"+m_password+".");
    /*KMessageBox u;
    u.about(new QWidget(),"p:"+m_password +"\np:"+m_crypto.decryptToString(m_password),"dd");*/
    //KMessageBox::about(0,m_password,"dd");
    ui.currentPasswordLine->setEnabled(!m_password.isEmpty());
   

    connect(ui.fontStyleComboBox, SIGNAL(currentFontChanged(QFont)), parent, SLOT(settingsModified()));
    connect(ui.fontBoldCheckBox, SIGNAL(toggled(bool)), parent, SLOT(settingsModified()));
    connect(ui.fontItalicCheckBox, SIGNAL(toggled(bool)), parent, SLOT(settingsModified()));
    connect(ui.autoFontPercent, SIGNAL(valueChanged(int)), parent, SLOT(settingsModified()));
    connect(ui.autoFont, SIGNAL(clicked(bool)), parent, SLOT(settingsModified()));
    connect(ui.customFontSizeSpinBox, SIGNAL(valueChanged(int)), parent, SLOT(settingsModified()));
    connect(ui.textBackgroundColorButton, SIGNAL(changed(QColor)), parent, SLOT(settingsModified()));
    connect(ui.textColorButton, SIGNAL(changed(QColor)), parent, SLOT(settingsModified()));
    connect(ui.notesColorComboBox, SIGNAL(currentIndexChanged(int)), parent, SLOT(settingsModified()));
    /*connect(ui.checkSpelling, SIGNAL(toggled(bool)), parent, SLOT(settingsModified()));*/
    connect(ui.useThemeColor, SIGNAL(toggled(bool)), parent, SLOT(settingsModified()));
    connect(ui.useCustomColor, SIGNAL(toggled(bool)), parent, SLOT(settingsModified()));
    connect(ui.useCustomBackgroundColor, SIGNAL(toggled(bool)), parent, SLOT(settingsModified()));
    connect(ui.useNoColor, SIGNAL(toggled(bool)), parent, SLOT(settingsModified()));
    connect(ui.currentPasswordLine, SIGNAL(textChanged(QString)), parent, SLOT(settingsModified()));    
    connect(ui.newPasswordLine1, SIGNAL(textChanged(QString)), parent, SLOT(settingsModified()));
}

void NotasSeguras::configAccepted()
{
    KConfigGroup cg = config();
    bool changed = false;

    QFont newFont = ui.fontStyleComboBox->currentFont();
    newFont.setBold(ui.fontBoldCheckBox->isChecked());
    newFont.setItalic(ui.fontItalicCheckBox->isChecked());

    //Apply bold and italic changes (if any) here (this is destructive formatting)
    bool boldChanged = (m_font.bold() != newFont.bold());
    bool italicChanged = (m_font.italic() != newFont.italic());
    if (boldChanged || italicChanged) {
        //Save previous selection
        QTextCursor oldCursor = m_noteEditor->textCursor();
        m_noteEditor->selectAll();
        if (boldChanged) {
            m_noteEditor->setFontWeight(newFont.weight());
        }
        if (italicChanged) {
            m_noteEditor->setFontItalic(newFont.italic());
        }
        //Restore previous selection
        m_noteEditor->setTextCursor(oldCursor);
    }

    //Save font settings to config
    if (m_font != newFont) {
        changed = true;
        cg.writeEntry("font", newFont);
        m_font = newFont;

        //Apply font family
        QTextCursor oldCursor = m_noteEditor->textCursor();
        m_noteEditor->selectAll();
        m_noteEditor->setFontFamily(m_font.family());
        m_noteEditor->setTextCursor(oldCursor);
    }

    if (m_customFontSize != ui.customFontSizeSpinBox->value()) {
        changed = true;
        m_customFontSize = ui.customFontSizeSpinBox->value();
        cg.writeEntry("customFontSize", m_customFontSize);
    }

    if (m_autoFont != ui.autoFont->isChecked()) {
        changed = true;
        m_autoFont = ui.autoFont->isChecked();
        cg.writeEntry("autoFont", m_autoFont);
    }

    if (m_autoFontPercent != ui.autoFontPercent->value()) {
        changed = true;
        m_autoFontPercent = (ui.autoFontPercent->value());
        cg.writeEntry("autoFontPercent", m_autoFontPercent);
    }

    disconnect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeChanged()));
    bool textColorChanged = false;
    if (m_useThemeColor != ui.useThemeColor->isChecked()) {
        changed = true;
        textColorChanged = true;
        m_useThemeColor = ui.useThemeColor->isChecked();
        cg.writeEntry("useThemeColor", m_useThemeColor);
    }

    if (m_useThemeColor) {
        m_textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
        connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeChanged()));
    } else {
        const QColor newColor = ui.textColorButton->color();
        if (m_textColor != newColor) {
            changed = true;
            textColorChanged = true;
            m_textColor = newColor;
            cg.writeEntry("textColor", m_textColor);
        }
    }

    if (textColorChanged) {
        QTextCursor oldCursor = m_noteEditor->textCursor();
        m_noteEditor->selectAll();
        m_noteEditor->setTextColor(m_textColor);
        m_noteEditor->setTextCursor(oldCursor);
    }

    if (m_useNoColor != ui.useNoColor->isChecked()) {
        changed = true;
        m_useNoColor = ui.useNoColor->isChecked();
        cg.writeEntry("useNoColor", m_useNoColor);
        QTextCursor textCursor = m_noteEditor->textCursor();
        QTextEdit::ExtraSelection textxtra;
        textxtra.cursor = m_noteEditor->textCursor();
        textxtra.cursor.movePosition( QTextCursor::StartOfLine );
        textxtra.cursor.movePosition( QTextCursor::EndOfLine, QTextCursor::KeepAnchor );
        textxtra.format.setBackground( Qt::transparent );

        QList<QTextEdit::ExtraSelection> extras;
        extras << textxtra;
        m_noteEditor->setExtraSelections( extras );
    }

    const QColor newBackgroundColor = ui.textBackgroundColorButton->color();
    if (m_textBackgroundColor != newBackgroundColor) {
        changed = true;
        m_textBackgroundColor = newBackgroundColor;
        cg.writeEntry("textBackgroundColor", m_textBackgroundColor);
    }

    /*bool spellCheck = ui.checkSpelling->isChecked();
    if (spellCheck != m_checkSpelling) {
      changed = true;
        m_checkSpelling = spellCheck;
        cg.writeEntry("checkSpelling", m_checkSpelling);
        m_noteEditor->setCheckSpellingEnabled(m_checkSpelling);
    }*/

    QList<QAction *> colorActions = m_colorMenu->actions();
    QAction *colorAction = colorActions.value(ui.notesColorComboBox->currentIndex());
    if (colorAction) {
        const QString tmpColor = colorAction->property("color").toString();
        if (tmpColor != m_topWidget->color()){
            m_topWidget->setColor(tmpColor);
            cg.writeEntry("color", m_topWidget->color());
            changed = true;
        }
    }
    //showMessage(KIcon("dialog-error"), ui.newPasswordLine1->text()+" " +ui.newPasswordLine2->text(), Plasma::ButtonOk);
    //bool match=false;
   if(!ui.newPasswordLine1->text().isEmpty() || !ui.newPasswordLine2->text().isEmpty()){
     if(ui.newPasswordLine1->text()==ui.newPasswordLine2->text()){
       QString currentPassword=ui.currentPasswordLine->text();
       
      /* KMessageBox *uno= new KMessageBox;    
	  uno->about(new QWidget(),ui.newPasswordLine1->text()+" " +ui.newPasswordLine2->text()+ " currentPassword:"+currentPassword+" m_password:"+m_password,"prueba");*/
       //KMessageBox::error(0,m_textEncrypted);
       //primera vez o cambio de password
	if((m_password.isEmpty() && currentPassword.isEmpty()) || (!currentPassword.isEmpty() && currentPassword==m_password)){
	  //QString texto=m_crypto.decryptToString(m_textEncrypted);
	  //m_text=m_noteEditor->textOrHtml();
	  qsrand(QTime::currentTime().msec());
	  numberkey=qrand();
	  m_crypto.setKey(numberkey);
	  m_password=m_crypto.encryptToString(ui.newPasswordLine1->text());
	  m_textEncrypted=m_crypto.encryptToString(m_text);
	  cg.writeEntry("autoSaveHtml", m_textEncrypted);
	  cg.writeEntry("keyPassword", m_password);
	  cg.writeEntry("numberKey", numberkey);
	   
	  m_noteEditor->setTextOrHtml("");
	  m_lineEdit->setText("");
	   //Plasma::Dialog *dialog= new Plasma::Dialog(m_topWidget);
	  changed = true;
	   setConfigurationRequired(FALSE,i18n("Password was set correctly"));
	  //match=true;
	}else{
	  //showMessage();
	  //showMessage(KIcon("dialog-error"), i18n("Passwords do not match"), Plasma::ButtonOk);
	   //showMessage();
	   KMessageBox::error(0,i18n("The password is incorrect"));
	  
	   //setConfigurationRequired(true);
	   //setConfigurationRequired(true,i18n("Passwords do not match"));
	}
    }else{
      KMessageBox::error(0,i18n("Passwords do not match"));
      //showMessage(KIcon("dialog-error"), i18n("The password is incorrect"), Plasma::ButtonOk);
      //setConfigurationRequired(true);
       //setConfigurationRequired(true,i18n("The password is incorrect") +ui.newPasswordLine1->text()+" " +ui.newPasswordLine2->text());
    }
     
  }
   //KMessageBox::error(0,m_password);
  /*if(match){
   
  }*/
   
    
      //showMessage(KIcon("dialog-error"), m_topWidget->wi->imagePath(), Plasma::ButtonOk);

    if (changed) {
        updateTextGeometry();
        update();
        emit configNeedsSaving();
    }
     //KMessageBox::error(0,"xx text:"+m_textEncrypted+"\n t:"+m_text+"");
    
}

void NotasSeguras::createTextFormatingWidgets()
{
    m_formatMenu = new QMenu(i18n("Formatting"));
    m_noteEditor->setFormatMenu(m_formatMenu);
    QAction *actionBold = m_formatMenu->addAction(KIcon("format-text-bold"), i18n("Bold"));
    QAction *actionItalic = m_formatMenu->addAction(KIcon("format-text-italic"),i18n("Italic"));
    QAction *actionUnderline = m_formatMenu->addAction(KIcon("format-text-underline"), i18n("Underline"));
    QAction *actionStrikeThrough = m_formatMenu->addAction(KIcon("format-text-strikethrough"), i18n("StrikeOut"));
    QAction *actionCenter = m_formatMenu->addAction(KIcon("format-justify-center"), i18n("Justify center"));
    QAction *actionFill = m_formatMenu->addAction(KIcon("format-justify-fill"), i18n("Justify"));
    connect(actionItalic, SIGNAL(triggered()), m_noteEditor, SLOT(italic()));
    connect(actionBold, SIGNAL(triggered()), m_noteEditor, SLOT(bold()));
    connect(actionUnderline, SIGNAL(triggered()), m_noteEditor, SLOT(underline()));
    connect(actionStrikeThrough, SIGNAL(triggered()), m_noteEditor, SLOT(strikeOut()));
    connect(actionCenter, SIGNAL(triggered()), m_noteEditor, SLOT(justifyCenter()));
    connect(actionFill, SIGNAL(triggered()), m_noteEditor, SLOT(justifyFill()));
    connect(actionItalic, SIGNAL(triggered()), this, SLOT(updateOptions()));
    connect(actionBold, SIGNAL(triggered()), this, SLOT(updateOptions()));
    connect(actionUnderline, SIGNAL(triggered()), this, SLOT(updateOptions()));
    connect(actionStrikeThrough, SIGNAL(triggered()), this, SLOT(updateOptions()));
    connect(actionCenter, SIGNAL(triggered()), this, SLOT(updateOptions()));
    connect(actionFill, SIGNAL(triggered()), this, SLOT(updateOptions()));

    widget = new QGraphicsWidget(m_topWidget);
    widget->setMaximumHeight(25);

    QGraphicsLinearLayout *buttonLayout = new QGraphicsLinearLayout(Qt::Horizontal, widget);
    buttonLayout->setSpacing(0);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    widget->setLayout(buttonLayout);

    m_buttonOption = new Plasma::ToolButton(widget);
    m_buttonOption->nativeWidget()->setIcon(KIcon("draw-text"));
    m_buttonOption->nativeWidget()->setCheckable(true);

    m_buttonBold = new Plasma::ToolButton(widget);
    m_buttonBold->setAction(actionBold);

    m_buttonItalic = new Plasma::ToolButton(widget);
    m_buttonItalic->setAction(actionItalic);

    m_buttonUnderline = new Plasma::ToolButton(widget);
    m_buttonUnderline->setAction(actionUnderline);

    m_buttonStrikeThrough = new Plasma::ToolButton(widget);
    m_buttonStrikeThrough->setAction(actionStrikeThrough);

    m_buttonCenter = new Plasma::ToolButton(widget);
    m_buttonCenter->setAction(actionCenter);

    m_buttonFill = new Plasma::ToolButton(widget);
    m_buttonFill->setAction(actionFill);

    buttonLayout->addItem(m_buttonOption);
    buttonLayout->addStretch();
    buttonLayout->addItem(m_buttonBold);
    buttonLayout->addItem(m_buttonItalic);
    buttonLayout->addItem(m_buttonUnderline);
    buttonLayout->addItem(m_buttonStrikeThrough);
    buttonLayout->addItem(m_buttonCenter);
    buttonLayout->addItem(m_buttonFill);
    buttonLayout->addStretch();
    
    m_layout->addItem(widget);

    m_buttonAnimGroup = new QParallelAnimationGroup(this);

    for (int i = 0; i < 6; i++){
        m_buttonAnim[i] = Plasma::Animator::create(Plasma::Animator::FadeAnimation, this);
        m_buttonAnimGroup->addAnimation(m_buttonAnim[i]);
    }

    m_buttonAnim[0]->setTargetWidget(m_buttonBold);
    m_buttonAnim[1]->setTargetWidget(m_buttonItalic);
    m_buttonAnim[2]->setTargetWidget(m_buttonUnderline);
    m_buttonAnim[3]->setTargetWidget(m_buttonStrikeThrough);
    m_buttonAnim[4]->setTargetWidget(m_buttonCenter);
    m_buttonAnim[5]->setTargetWidget(m_buttonFill);

    showOptions(false);
    connect(m_buttonOption->nativeWidget(), SIGNAL(toggled(bool)), this, SLOT(showOptions(bool)));

    connect(m_noteEditor, SIGNAL(cursorPositionChanged()), this, SLOT(updateOptions()));
    
}

void NotasSeguras::showOptions(bool show)
{
    m_buttonOption->nativeWidget()->setDown(show);

    qreal targetOpacity = show ? 1 : 0;
    qreal startOpacity = 1 - targetOpacity;

    for (int i = 0; i < 6; i++){
        m_buttonAnim[i]->setProperty("startOpacity", startOpacity);
        m_buttonAnim[i]->setProperty("targetOpacity", targetOpacity);
    }

    m_buttonAnimGroup->start();
}

void NotasSeguras::updateOptions()
{
    m_buttonBold->setDown(m_noteEditor->fontWeight() == QFont::Bold);
    m_buttonItalic->setDown(m_noteEditor->fontItalic());
    m_buttonUnderline->setDown(m_noteEditor->fontUnderline());
    m_buttonStrikeThrough->setDown(m_noteEditor->currentFont().strikeOut());
    m_buttonCenter->setDown(m_noteEditor->alignment() == Qt::AlignHCenter);
    m_buttonFill->setDown(m_noteEditor->alignment() == Qt::AlignJustify);
}

#include "notasseguras.moc"

//K_EXPORT_PLASMA_APPLET(notes, Notes)
K_EXPORT_PLASMA_APPLET(notasseguras, NotasSeguras)

