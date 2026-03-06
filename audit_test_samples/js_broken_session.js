function checkLogin(user, password) {
    if (user === "admin" && password === "admin123") {
        // Insecure: Session ID is just a random number, easy to predict
        const sessionId = Math.floor(Math.random() * 1000);
        document.cookie = "session_id=" + sessionId;
        return true;
    }
    return false;
}
