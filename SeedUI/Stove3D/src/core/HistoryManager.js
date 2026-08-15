/**
 * HistoryManager provides command-based Undo / Redo supporting:
 * 1. Object Transforms (Translate / Rotate / Scale).
 * 2. Mesh Sub-element edits (Vertex / Edge / Face movements).
 * 3. Object Creation / Duplication.
 * 4. Object Deletion.
 * 
 * Supports both standard shortcuts:
 * - Ctrl + Z (Undo)
 * - Ctrl + Shift + Z & Ctrl + Y (Redo)
 */
export class HistoryManager {
  constructor(maxHistory = 60) {
    this.undoStack = [];
    this.redoStack = [];
    this.maxHistory = maxHistory;
    this.onHistoryChange = null;
  }

  push(action) {
    if (!action || typeof action.undo !== 'function' || typeof action.redo !== 'function') return;
    this.undoStack.push(action);
    if (this.undoStack.length > this.maxHistory) {
      this.undoStack.shift();
    }
    this.redoStack = [];
    if (this.onHistoryChange) this.onHistoryChange();
  }

  undo() {
    if (this.undoStack.length === 0) return false;
    const action = this.undoStack.pop();
    try {
      action.undo();
      this.redoStack.push(action);
      if (this.onHistoryChange) this.onHistoryChange();
      return true;
    } catch (err) {
      console.error('Erro ao executar Undo:', err);
      return false;
    }
  }

  redo() {
    if (this.redoStack.length === 0) return false;
    const action = this.redoStack.pop();
    try {
      action.redo();
      this.undoStack.push(action);
      if (this.onHistoryChange) this.onHistoryChange();
      return true;
    } catch (err) {
      console.error('Erro ao executar Redo:', err);
      return false;
    }
  }

  canUndo() {
    return this.undoStack.length > 0;
  }

  canRedo() {
    return this.redoStack.length > 0;
  }

  clear() {
    this.undoStack = [];
    this.redoStack = [];
    if (this.onHistoryChange) this.onHistoryChange();
  }
}
