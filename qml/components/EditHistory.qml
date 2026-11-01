/*!
    \qmltype EditHistory
    \brief Step history for a text editor that has no undo of its own.

    Silica's TextArea exposes neither \c undo() nor \c redo(), so the steps are
    kept here. A burst of typing collapses into one step: the first change
    records the text as it was, and the step closes once typing pauses.

    The history does not know what it is editing. The caller hands in the
    editor as \c target and reports changes through \c watch().

    \qml
    EditHistory {
        id: history
        target: bodyField
    }

    // bodyField.onTextChanged: history.watch(text, cursorPosition)
    // undoButton.onClicked: history.undo()
    \endqml
*/
import QtQuick 2.0

QtObject {
    id: root

    property Item target
    // How long typing has to pause before the current step is closed.
    property int burstInterval: 400
    property int depth: 50
    readonly property bool canUndo: undoCount > 0
    readonly property bool canRedo: redoCount > 0
    property int undoCount: 0
    property int redoCount: 0
    property var undoStack: []
    property var redoStack: []
    // The editor as of the last reported change, which is the state an undo
    // steps away from.
    property string previousText: ""
    property int previousCursor: 0
    property bool applying: false
    property bool bursting: false
    property Timer burstTimer

    burstTimer: Timer {
        interval: root.burstInterval
        onTriggered: root.bursting = false
    }

    function reset(text, cursor) {
        undoStack = [];
        redoStack = [];
        undoCount = 0;
        redoCount = 0;
        bursting = false;
        previousText = text;
        previousCursor = cursor;
    }

    function snapshot() {
        return {
            "text": previousText,
            "cursor": previousCursor
        };
    }

    function pushUndo(step) {
        undoStack.push(step);
        if (undoStack.length > depth)
            undoStack.shift();

        undoCount = undoStack.length;
    }

    function pushRedo(step) {
        redoStack.push(step);
        redoCount = redoStack.length;
    }

    function clearRedo() {
        redoStack = [];
        redoCount = 0;
    }

    function watch(text, cursor) {
        if (applying) {
            remember(text, cursor);
            return ;
        }
        if (!bursting) {
            pushUndo(snapshot());
            clearRedo();
            bursting = true;
        }
        remember(text, cursor);
        burstTimer.restart();
    }

    function remember(text, cursor) {
        previousText = text;
        previousCursor = cursor;
    }

    // Ends the current step, so what follows is recorded on its own.
    function seal() {
        burstTimer.stop();
        bursting = false;
    }

    function undo() {
        if (!canUndo)
            return ;

        seal();
        pushRedo(snapshot());
        apply(undoStack.pop());
        undoCount = undoStack.length;
    }

    function redo() {
        if (!canRedo)
            return ;

        seal();
        pushUndo(snapshot());
        apply(redoStack.pop());
        redoCount = redoStack.length;
    }

    function apply(step) {
        if (!target)
            return ;

        applying = true;
        target.text = step.text;
        target.cursorPosition = Math.min(step.cursor, step.text.length);
        applying = false;
        remember(step.text, step.cursor);
    }

}
