#pragma once

#include "QtMetacallAdapter.h"

#include <QtCore/QEvent>
#include <QtCore/QVector>

/** QtSignalForwarder provides a way to connect Qt signals to QtCallback objects
 * or function objects (wrappers around functions such as std::tr1::function,
 * boost::function or std::function).
 * 
 * In Qt 5, this is supported natively using the new signal and slot syntax:
 * http://qt-project.org/wiki/New_Signal_Slot_Syntax .  This is especially useful
 * when combined with C++11's lambdas.
 *
 * QtSignalForwarder provides a way to simulate this under Qt 4 with C++03 by installing
 * a proxy object between the original sender of the signal and the receiver.  The proxy
 * receives the signal and then invokes the callback functor with the signal's arguments.
 *
 * Checking of signal and receiver argument types is done at runtime when setting up
 * a connection, as with normal signals and slots in Qt 4.
 *
 * Example usage, binding a signal with no arguments to a callback which invokes
 * a slot with one fixed argument:
 *
 *  MyObject receiver;
 *  QPushButton button;
 *  QtCallback callback(&receiver, SLOT(buttonClicked(int)));
 *  callback.bind(42);
 *  QtSignalForwarder::connect(&button, SIGNAL(clicked(bool)), callback);
 *  button.click();
 *
 * Will invoke Receiver::buttonClicked(42)
 *
 * You can also use QtSignalForwarder to invoke callbacks when an object receives an event
 * (as opposed to emits a signal).  For example:
 *
 *   QtSignalForwarder::connect(widget, QEvent::Enter,
 *     QtCallback(otherWidget, SLOT(setVisible(bool))).bind(true));
 *   QtSignalForwarder::connect(widget, QEvent::Leave,
 *     QtCallback(otherWidget, SLOT(setVisible(bool))).bind(false));
 *
 * Will show 'otherWidget' when the mouse hovers over 'widget' and hide it otherwise.
 */
class QtSignalForwarder : public QObject
{
	// no Q_OBJECT macro here - see qt_metacall() re-implementation
	// below
	
	public:
		using QObject::connect;
		using QObject::disconnect;

		typedef bool (*EventFilterFunc)(QObject*,QEvent*);

		QtSignalForwarder(QObject* parent = 0);
		virtual ~QtSignalForwarder();

		/** Set up a binding so that @p callback is invoked when
		 * @p sender emits @p signal.  If @p signal has default arguments,
		 * they must be specified.  eg. Use SLOT(clicked(bool)) for a button
		 * rather than SLOT(clicked()).
		 *
		 * The @p callback argument can be a QtCallback object or a function
		 * object (eg. std::tr1::function).
		 *
		 * The connection will automatically disconnect if the sender or the
		 * @p context context is destroyed.
		 */
		bool bind(QObject* sender, const char* signal, QObject *context,
			const QtMetacallAdapter& callback
		);
		bool bind(QObject* sender, const char* signal, const QtMetacallAdapter& callback)
		{
			return bind(sender, signal, 0, callback);
		}

		/** Set up a binding so that @p callback is invoked when @p sender
		 * receives @p event.
		 */
		bool bind(QObject* sender, QEvent::Type event, const QtMetacallAdapter& callback, EventFilterFunc filter = 0);

		/** Remove all bindings from a given @p sender and signal. */
		void unbind(QObject* sender, const char* signal);

		/** Remove all bindings from a given @p sender and event. */
		void unbind(QObject* sender, QEvent::Type event);

		/** Remove all bindings from a given @p sender. */
		void unbind(QObject* sender);

		/** Returns the total number of active bindings for this
		 * QtSignalForwarder instance.
		 */
		int bindingCount() const;

		bool isConnected(QObject* sender) const;

		/** Schedule a delayed call to @p callback after @p minDelay ms.
		 *
		 * The connection will automatically disconnect if the
		 * @p context context is destroyed.
		 */
		static void delayedCall(int minDelay, QObject *context,
			const QtMetacallAdapter& callback
		);
		static void delayedCall(int minDelay, const QtMetacallAdapter& callback)
		{
			delayedCall(minDelay, 0, callback);
		}

		// re-implemented from QObject (this method is normally declared via the Q_OBJECT
		// macro and implemented by the code generated by moc)
		virtual int qt_metacall(QMetaObject::Call call, int methodId, void** arguments);

		// note: The static connect() and disconnect() functions may currently only
		// be used from the main application thread

		/** Install a proxy which invokes @p callback when @p sender emits @p signal.
		 *
		 * The connection will automatically disconnect if the sender or the
		 * @p context context is destroyed.
		 */
		static bool connect(QObject* sender, const char* signal, QObject *context,
			const QtMetacallAdapter& callback
		);
		static bool connect(QObject* sender, const char* signal,
			const QtMetacallAdapter& callback
		)
		{
			return connect(sender, signal, 0, callback);
		}

		static void disconnect(QObject* sender, const char* signal);

		/** Install a proxy which invokes @p callback when @p sender receives @p event.
		 */
		static bool connect(QObject* sender, QEvent::Type event, const QtMetacallAdapter& callback, EventFilterFunc filter = 0);
		static void disconnect(QObject* sender, QEvent::Type event);

		/** Convenience method which connects a signal to a slot which takes a pointer
		 * to the sender as the first argument. This can be used as an alternative to explicitly checking
		 * the sender in the slot itself or using QSignalMapper.
		 *
		 * In the current implementation, the receiver type must be registered using
		 * qRegisterMetaType<T>().
		 *
		 * For example: connectWithSender(myButton, SIGNAL(clicked()), myForm, SLOT(buttonClicked(QPushButton*)))
		 */
		static bool connectWithSender(QObject* sender, const char* signal, QObject* receiver, const char* slot);

		// re-implemented from QObject
		virtual bool eventFilter(QObject* watched, QEvent* event);

	private:
		struct Binding
		{
			Binding(QObject* _sender = 0, int _signalIndex = -1,
				QObject *_context = 0,
				const QtMetacallAdapter& _callback = QtMetacallAdapter()
			)
				: sender(_sender)
				, context(_context)
				, signalIndex(_signalIndex)
				, callback(_callback)
			{}

			const char* paramType(int index) const
			{
				if (index >= paramTypes.count()) {
					return 0;
				}
				return paramTypes.at(index).constData();
			}

			QObject* sender;
			QObject* context;
			int signalIndex;
			QList<QByteArray> paramTypes;
			QtMetacallAdapter callback;
		};

		struct EventBinding
		{
			EventBinding(QObject* _sender = 0, QEvent::Type _type = QEvent::None, const QtMetacallAdapter& _callback = QtMetacallAdapter(),
			             EventFilterFunc _filter = 0)
				: sender(_sender)
				, eventType(_type)
				, filter(_filter)
				, callback(_callback)
			{}

			QObject* sender;
			QEvent::Type eventType;
			EventFilterFunc filter;
			QtMetacallAdapter callback;
		};

		// returns the first binding for (sender, signalIndex)
		const Binding* matchBinding(QObject* sender, int signalIndex) const;
		void failInvoke(const QString& error);
		void setupDestroyNotify(QObject* sender);

		// returns false if the limit on the number of signal bindings
		// per proxy has been reached
		bool canAddSignalBindings() const;

		static bool checkTypeMatch(const QtMetacallAdapter& callback, const QList<QByteArray>& paramTypes);
		static QtSignalForwarder* sharedProxy(QObject* sender);
		static void invokeBinding(const Binding& binding, void** arguments);

		// map of sender -> signal binding IDs
		QMultiHash<QObject*,int> m_senderSignalBindingIds;
		// map of context -> signal binding IDs
		QMultiHash<QObject*,int> m_contextBindingIds;
		// map of binding ID -> binding
		QHash<int,Binding> m_signalBindings;
		QHash<QObject*,EventBinding> m_eventBindings;

		// list of available method IDs for new signal
		// bindings
		QList<int> m_freeSignalBindingIds;

		// a sentinel callback object for use with the automatically created
		// bindings to QObject::destroy(QObject*) used to detect when a bound
		// sender is destroyed
		static QtMetacallAdapter s_senderDestroyedCallback;
};

Q_DECLARE_METATYPE(QtSignalForwarder*)

