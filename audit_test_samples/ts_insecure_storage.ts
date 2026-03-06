function saveUserSession(token: string, userId: string) {
    // Insecure storage: sensitive info in localStorage
    localStorage.setItem('auth_token', token);
    localStorage.setItem('user_id', userId);
}

function getUserToken() {
    return localStorage.getItem('auth_token');
}
