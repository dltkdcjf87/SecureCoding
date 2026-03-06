import java.io.File;
import javax.servlet.http.HttpServletRequest;

public class FileDownload {
    public void downloadFile(HttpServletRequest request) {
        String fileName = request.getParameter("filename");
        // Path Traversal: fileName is not validated
        File file = new File("/var/www/uploads/" + fileName);
        if (file.exists()) {
            // transmit file...
        }
    }
}
