#include <CQDiff.h>
#include <CQApp.h>
#include <iostream>

int
main(int argc, char **argv)
{
  CQApp app(argc, argv);

  if (argc != 3) {
    std::cerr << "Usage:: CQDiff <file1> <file2>" << std::endl;
    exit(1);
  }

  CQDiff *diff = new CQDiff;

  diff->init();

  diff->setFiles(argv[1], argv[2]);

  diff->show();

  app.exec();

  return 0;
}
