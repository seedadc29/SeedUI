export class HistoryManager {
  constructor(getStateFn = null, restoreStateFn = null) {
    this.getStateFn = getStateFn;
    this.restoreStateFn = restoreStateFn;
    this.undoStack = [];
    this.redoStack = [];
    this.maxHistory = 60;
    this.isRestoring = false;
  }

  saveSnapshot() {
    if (this.isRestoring || !this.getStateFn) return;
    try {
      const state = this.getStateFn();
      if (!state) return;
      const serialized = JSON.stringify(state);

      // Prevent duplicate identical states on consecutive calls
      if (this.undoStack.length > 0 && this.undoStack[this.undoStack.length - 1] === serialized) {
        return;
      }

      this.undoStack.push(serialized);
      if (this.undoStack.length > this.maxHistory) {
        this.undoStack.shift();
      }
      this.redoStack = [];
    } catch (err) {
      console.warn('History save error:', err);
    }
  }

  pushState(state) {
    if (this.getStateFn) {
      this.saveSnapshot();
      return;
    }
    // Fallback direct state push
    try {
      const snapshot = JSON.stringify(state, (k, v) => {
        if (k === 'parentSun' || k === 'parentPlanet') return undefined;
        return v;
      });
      if (!snapshot) return;
      this.undoStack.push(snapshot);
      if (this.undoStack.length > this.maxHistory) {
        this.undoStack.shift();
      }
      this.redoStack = [];
    } catch (e) {}
  }

  undo() {
    if (this.undoStack.length === 0) return null;

    try {
      if (this.getStateFn && this.restoreStateFn) {
        const currentState = this.getStateFn();
        this.redoStack.push(JSON.stringify(currentState));

        const previousSerialized = this.undoStack.pop();
        if (!previousSerialized) return null;

        const state = JSON.parse(previousSerialized);
        this.isRestoring = true;
        this.restoreStateFn(state);
        this.isRestoring = false;
        return state;
      }
    } catch (e) {
      console.error('Undo error:', e);
      this.isRestoring = false;
      return null;
    }

    // Direct fallback
    const prev = this.undoStack.pop();
    if (prev) {
      try {
        return JSON.parse(prev);
      } catch (e) {
        return null;
      }
    }
    return null;
  }

  redo() {
    if (this.redoStack.length === 0) return null;

    try {
      if (this.getStateFn && this.restoreStateFn) {
        const currentState = this.getStateFn();
        this.undoStack.push(JSON.stringify(currentState));

        const nextSerialized = this.redoStack.pop();
        if (!nextSerialized) return null;

        const state = JSON.parse(nextSerialized);
        this.isRestoring = true;
        this.restoreStateFn(state);
        this.isRestoring = false;
        return state;
      }
    } catch (e) {
      console.error('Redo error:', e);
      this.isRestoring = false;
      return null;
    }

    // Direct fallback
    const next = this.redoStack.pop();
    if (next) {
      try {
        return JSON.parse(next);
      } catch (e) {
        return null;
      }
    }
    return null;
  }

  canUndo() {
    return this.undoStack.length > 0;
  }

  canRedo() {
    return this.redoStack.length > 0;
  }
}
