package net.sourceforge.gemrb;

import org.libsdl.app.SDLActivity;
import android.os.Bundle;
import android.util.Log;
import java.io.IOException;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.BufferedOutputStream;
import java.io.BufferedInputStream;
import java.io.InputStream;
import android.content.res.AssetManager;
import java.util.Enumeration;
import java.util.zip.*;
import java.util.Properties;
import android.app.AlertDialog;
import android.widget.EditText;
import android.content.DialogInterface;
import android.content.res.Configuration;
import android.content.Environment;

public class GemRB extends SDLActivity {

  public String newPath;

  protected void onCreate(Bundle savedInstanceState) {
    File gemrbHomeFolder = getApplicationContext().getExternalFilesDir(null); // null for general purpose files
    String[] foldersToExtract = { "GUIScripts", "override", "unhardcoded" };

    // first clear the directories and their contents
    Log.d("GemRB Activity", "Cleaning up...");
    for(String folder : foldersToExtract) {
      deleteRecursive(new File(gemrbHomeFolder.getAbsolutePath().concat(gemrbHomeFolder.separator).concat(folder)));
    }

    Log.d("GemRB Activity", "Extracting new files...");
    try {
      extractFolder(this.getApplication().getPackageCodePath(), gemrbHomeFolder.getAbsolutePath());
    } catch (Exception e) {
      throw new RuntimeException(e);
    }
    Log.d("GemRB Activity", "Done.");

    Log.d("GemRB Activity", "Checking GemRB.cfg content.");

    File finalConfFile = new File(gemrbHomeFolder.getAbsolutePath().concat(gemrbHomeFolder.separator).concat("GemRB.cfg"));
    File userSuppliedConfig = new File(Environment.getExternalStorageDirectory().concat(gemrbHomeFolder.separator).concat("GemRB.cfg"));

    if(!finalConfFile.exists()) {
      Log.d("GemRB Activity", "GemRB.cfg doesn't exist in the expected location, creating it from the packaged template.");

      // String[] keysToChange = { "GUIScriptsPath", "GemRBOverridePath", "GemRBUnhardcodedPath" };
      try {
        File inConfFile = new File(gemrbHomeFolder.getAbsolutePath().concat(gemrbHomeFolder.separator).concat("packaged.GemRB.cfg"));
        BufferedReader inConf = new BufferedReader(new FileReader(inConfFile));
        File outConfFile = new File(gemrbHomeFolder.getAbsolutePath().concat(gemrbHomeFolder.separator).concat("new.GemRB.cfg"));
        BufferedWriter outConf = new BufferedWriter(new FileWriter(outConfFile));

        String line;

        while((line = inConf.readLine()) != null) {
          outConf.write(line.concat("\n"));
          outConf.flush();
        }
        inConf.close();
        outConf.flush();
        outConf.close();

        inConfFile.delete();
        outConfFile.renameTo(finalConfFile);
      } catch (IOException e) {
        throw new RuntimeException(e);
      }
    }

    super.onCreate(savedInstanceState);
  }

  private static void deleteRecursive(File fileOrDirectory) {
      if (fileOrDirectory.isDirectory())
          for (File child : fileOrDirectory.listFiles())
              deleteRecursive(child);

      fileOrDirectory.delete();
  }

  private static void extractFolder(String zipFile, String newPath) throws ZipException, IOException {
      int BUFFER = 1024 * 1024;
      File file = new File(zipFile);

      ZipFile zip = new ZipFile(file);

      new File(newPath).mkdir();
      Enumeration zipFileEntries = zip.entries();

      // establish buffer for writing file
      byte data[] = new byte[BUFFER];
      // Process each entry
      while (zipFileEntries.hasMoreElements()) {
          // grab a zip file entry
          ZipEntry entry = (ZipEntry) zipFileEntries.nextElement();
          String currentEntry = entry.getName();
          if(!currentEntry.contains("asset")) {
            continue;
          }
          File destFile = new File(newPath, currentEntry.substring(7)); // 6th index should be "/" of "assets/"
          //destFile = new File(newPath, destFile.getName());
          File destinationParent = destFile.getParentFile();

          // create the parent directory structure if needed
          destinationParent.mkdirs();

          if (!entry.isDirectory()) {
              InputStream is = zip.getInputStream(entry);
              int currentByte;

              // write the current file to disk
              FileOutputStream dest = new FileOutputStream(destFile);

              // read and write until last byte is encountered
              while ((currentByte = is.read(data, 0, BUFFER)) != -1) {
                  dest.write(data, 0, currentByte);
              }
              dest.flush();
              dest.close();
              is.close();
          }
      }
  }

  public void onConfigurationChanged(Configuration newConfig) {
    // we're only overriding for orientation change (cmp AndroidManifest.xml)
    // but we don't actually want to react to that
    super.onConfigurationChanged(newConfig);
  }
}
