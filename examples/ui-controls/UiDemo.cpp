#include "QtSignalForwarder.h"

#include <QApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

bool matchRightClick(QObject*, QEvent* event)
{
	return static_cast<QMouseEvent*>(event)->button() == Qt::RightButton;
}

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	QWidget* widget = new QWidget;
	
	QVBoxLayout* layout = new QVBoxLayout(widget);

	QPushButton* button1 = new QPushButton("Set to 10%");
	QPushButton* button2 = new QPushButton("Set to 50%");
	QPushButton* button3 = new QPushButton("Set to 80%");

	QLabel* hideMeLabel = new QLabel("Right click me to close");

	QLabel* focusLabel = new QLabel("Slider is hovered");
	focusLabel->setVisible(false);
	QSlider* slider = new QSlider(Qt::Horizontal);

	// create a callback object which sets the slider's value
	// and use QtSignalForwarder to invoke it when the button is
	// clicked
	QtCallback button1Callback(slider, SLOT(setValue(int)));
	button1Callback.bind(10);
	QtSignalForwarder::connect(button1, SIGNAL(clicked(bool)), button1Callback);

	// setup another couple of buttons.
	// Here we use a more succinct syntax to create the callback
	QtSignalForwarder::connect(button2, SIGNAL(clicked(bool)),
	  QtCallback(slider, SLOT(setValue(int))).bind(50));

#ifdef ENABLE_QTCALLBACK_TR1_FUNCTION
	// if tr1/function is available, use that to create a callback
	QtSignalForwarder::connect(button3, SIGNAL(clicked(bool)),
	  std::tr1::function<void()>(std::tr1::bind(&QAbstractSlider::setValue, slider, 80)));
#else
	QtSignalForwarder::connect(button3, SIGNAL(clicked(bool)),
	  QtCallback(slider, SLOT(setValue(int))).bind(80));
#endif

	QtSignalForwarder::connect(slider, QEvent::Enter,
	  QtCallback(focusLabel, SLOT(setVisible(bool))).bind(true));

	QtSignalForwarder::connect(slider, QEvent::Leave,
	  QtCallback(focusLabel, SLOT(setVisible(bool))).bind(false));

	QtSignalForwarder::connect(hideMeLabel, QEvent::MouseButtonRelease,
	  QtCallback(widget, SLOT(close())), matchRightClick);

	layout->addWidget(button1);
	layout->addWidget(button2);
	layout->addWidget(button3);
	layout->addWidget(slider);
	layout->addWidget(focusLabel);
	
	layout->addStretch();
	
	layout->addWidget(hideMeLabel);

	widget->show();

	return app.exec();
}

