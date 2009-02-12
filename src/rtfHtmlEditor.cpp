#include "rtfHtmlEditor.h"

RtfHtmlEditor::RtfHtmlEditor(QWidget *parent)
 : KTextEdit(parent), c(0)
 {
     setPlainText(tr("This TextEdit provides autocompletions for words that have more than"
                     " 3 characters. You can trigger autocompletion using ") +
                     QKeySequence("Ctrl+E").toString(QKeySequence::NativeText));
 }
 
  RtfHtmlEditor::~RtfHtmlEditor()
 {
 }
 
  void RtfHtmlEditor::setCompleter(QCompleter *completer)
 {
     if (c)
         QObject::disconnect(c, 0, this, 0);

     c = completer;

     if (!c)
         return;

     c->setWidget(this);
     c->setCompletionMode(QCompleter::PopupCompletion);
     c->setCaseSensitivity(Qt::CaseInsensitive);
     QObject::connect(c, SIGNAL(activated(const QString&)),
                      this, SLOT(insertCompletion(const QString&)));
 }
 
  QCompleter *RtfHtmlEditor::completer() const
 {
     return c;
 }
 
 void RtfHtmlEditor::insertCompletion(const QString& completion)
 {
     if (c->widget() != this)
         return;
     QTextCursor tc = textCursor();
     int extra = completion.length() - c->completionPrefix().length();
     tc.movePosition(QTextCursor::Left);
     tc.movePosition(QTextCursor::EndOfWord);
     tc.insertText(completion.right(extra) + ": ;");
     tc.movePosition(QTextCursor::Left);
     setTextCursor(tc);
 }

 
  QString RtfHtmlEditor::textUnderCursor() const
 {
     QTextCursor tc = textCursor();
     tc.select(QTextCursor::WordUnderCursor);
     return tc.selectedText();
 }
 
  void RtfHtmlEditor::focusInEvent(QFocusEvent *e)
 {
     if (c)
         c->setWidget(this);
     QTextEdit::focusInEvent(e);
 }
 
  void RtfHtmlEditor::keyPressEvent(QKeyEvent *e)
 {
    /*if (((e->key() == Qt::Key_Enter) || (e->key() == Qt::Key_Return)) && (!c->popup()->isVisible())) {
      printf("Enter pressed \n");
      if (textUnderCursor().indexOf("{") != -1) {
	//insertBrace();
	return;
      }
      else {
	insertTabulation();
	//return;
      }
    }*/
    
     if (c && c->popup()->isVisible()) {
         // The following keys are forwarded by the completer to the widget
        switch (e->key()) {
        case Qt::Key_Enter:
	  return;
        case Qt::Key_Return:
	  return;
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
             e->ignore();
             return; // let the completer do default behavior
        default:
            break;
        }
     }

     bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E); // CTRL+E
     if (!c || !isShortcut) // dont process the shortcut when we have a completer
         QTextEdit::keyPressEvent(e);
          const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
     if (!c || (ctrlOrShift && e->text().isEmpty()))
         return;

     static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word
     bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
     QString completionPrefix = textUnderCursor();

     if (!isShortcut && (hasModifier || e->text().isEmpty()|| completionPrefix.length() < 3
                       || eow.contains(e->text().right(1)))) {
         c->popup()->hide();
         return;
     }

     if (completionPrefix != c->completionPrefix()) {
         c->setCompletionPrefix(completionPrefix);
         c->popup()->setCurrentIndex(c->completionModel()->index(0, 0));
     }
     QRect cr = cursorRect();
     cr.setWidth(c->popup()->sizeHintForColumn(0)
                 + c->popup()->verticalScrollBar()->sizeHint().width());
     c->complete(cr); // popup it up!
 }
 
 