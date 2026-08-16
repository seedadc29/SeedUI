export class HistoryManager {
  constructor(onChange) {
    this.undoStack = [];
    this.redoStack = [];
    this.maxHistory = 50;
    this.onChange = onChange;
  }

  serialize(state) {
    try {
      return JSON.stringify(state, (key, value) => {
        if (key === 'parentSun' || key === 'parentPlanet') return undefined;
        return value;
      });
    } catch (err) {
      console.warn('Serialization fallback:', err);
      return null;
    }
  }

  pushState(state) {
    const snapshot = this.serialize(state);
    if (!snapshot) return;

    this.undoStack.push(snapshot);
    if (this.undoStack.length > this.maxHistory) {
      this.undoStack.shift();
    }
    this.redoStack = [];
  }

  undo(currentState) {
    if (this.undoStack.length === 0) return null;
    const currentSnapshot = this.serialize(currentState);
    if (currentSnapshot) this.redoStack.push(currentSnapshot);

    const previousSnapshot = this.undoStack.pop();
    if (!previousSnapshot) return null;

    try {
      const restoredState = JSON.parse(previousSnapshot);
      if (this.onChange) this.onChange(restoredState);
      return restoredState;
    } catch (e) {
      return null;
    }
  }

  redo(currentState) {
    if (this.redoStack.length === 0) return null;
    const currentSnapshot = this.serialize(currentState);
    if (currentSnapshot) this.undoStack.push(currentSnapshot);

    const nextSnapshot = this.redoStack.pop();
    if (!nextSnapshot) return null;

    try {
      const restoredState = JSON.parse(nextSnapshot);
      if (this.onChange) this.onChange(restoredState);
      return restoredState;
    } catch (e) {
      return null;
    }
  }

  canUndo() {
    return this.undoStack.length > 0;
  }

  canRedo() {
    return this.redoStack.length > 0;
  }
}
