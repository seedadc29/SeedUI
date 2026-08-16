export class HistoryManager {
  constructor(onChange) {
    this.undoStack = [];
    this.redoStack = [];
    this.maxHistory = 50;
    this.onChange = onChange;
  }

  pushState(state) {
    // Deep clone state
    const snapshot = JSON.stringify(state);
    this.undoStack.push(snapshot);
    if (this.undoStack.length > this.maxHistory) {
      this.undoStack.shift();
    }
    this.redoStack = [];
  }

  undo(currentState) {
    if (this.undoStack.length === 0) return null;
    const currentSnapshot = JSON.stringify(currentState);
    this.redoStack.push(currentSnapshot);

    const previousSnapshot = this.undoStack.pop();
    const restoredState = JSON.parse(previousSnapshot);
    if (this.onChange) this.onChange(restoredState);
    return restoredState;
  }

  redo(currentState) {
    if (this.redoStack.length === 0) return null;
    const currentSnapshot = JSON.stringify(currentState);
    this.undoStack.push(currentSnapshot);

    const nextSnapshot = this.redoStack.pop();
    const restoredState = JSON.parse(nextSnapshot);
    if (this.onChange) this.onChange(restoredState);
    return restoredState;
  }

  canUndo() {
    return this.undoStack.length > 0;
  }

  canRedo() {
    return this.redoStack.length > 0;
  }
}
