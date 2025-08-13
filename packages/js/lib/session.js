const path = require('path');

// Load the native addon
const addon = require(path.join(__dirname, '../../../build/Release/hweb_addon.node'));

/**
 * Session management for persistent browser state
 * 
 * @example
 * const session = new Session('my-session');
 * await session.save();
 * const sessions = session.list();
 */
class Session {
    /**
     * Create a new Session instance
     * @param {string} [sessionName='default'] - Name of the session
     * @param {string} [sessionsDir='./sessions'] - Directory to store sessions
     */
    constructor(sessionName = 'default', sessionsDir = './sessions') {
        this.sessionName = sessionName;
        this.sessionsDir = sessionsDir;
        
        // Create native session manager
        this._session = new addon.Session(sessionName, sessionsDir);
    }
    
    /**
     * Get the session name
     * @returns {string} - Session name
     */
    getName() {
        return this._session.getSessionName();
    }
    
    /**
     * Save the current session state
     * @returns {Promise<boolean>} - True if save succeeded
     */
    async save() {
        return new Promise((resolve, reject) => {
            try {
                const result = this._session.saveSession();
                resolve(result);
            } catch (error) {
                reject(error);
            }
        });
    }
    
    /**
     * Load session state
     * @returns {Promise<boolean>} - True if load succeeded
     */
    async load() {
        return new Promise((resolve, reject) => {
            try {
                const result = this._session.loadSession();
                resolve(result);
            } catch (error) {
                reject(error);
            }
        });
    }
    
    /**
     * Delete the session
     * @returns {Promise<boolean>} - True if deletion succeeded
     */
    async delete() {
        return new Promise((resolve, reject) => {
            try {
                const result = this._session.deleteSession();
                resolve(result);
            } catch (error) {
                reject(error);
            }
        });
    }
    
    /**
     * List all available sessions
     * @returns {Array<string>} - Array of session names
     */
    list() {
        return this._session.listSessions();
    }
    
    /**
     * Check if session exists
     * @returns {boolean} - True if session exists
     */
    exists() {
        const sessions = this.list();
        return sessions.includes(this.sessionName);
    }
}

module.exports = Session;