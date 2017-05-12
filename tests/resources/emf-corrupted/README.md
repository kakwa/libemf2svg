# List of corrupted files:

| FILE                                | CORRUPTED RECORD                                           |
|-------------------------------------|------------------------------------------------------------|
| bad_corrupted_2014-12-02-215338.emf | corrupted EMR_HEADER                                       |
| bad_corrupted_2014-12-02-215339.emf | corrupted EMR_RESTOREDC (wrong index)                      |
| bad_corrupted_2014-12-02-215358.emf | corrupted EMR_EXTTEXTOUTW (false text length)              |
| bad_corrupted_2014-12-02-215400.emf | corrupted U_EMR_HEADER (bad offDescription)                |
| bad_corrupted_2014-12-02-215425.emf | corrupted EMR_POLYLINE16 (bad cpts)                        |
| bad_corrupted_2014-12-02-215428.emf | corrupted EMR_HEADER                                       |
| bad_corrupted_2014-12-07-140817.emf | corrupted EMR_SELECTOBJECT                                 |
| bad_corrupted_2014-12-07-160856.emf | corrupted EMF_BEGINPATH                                    |
| bad_corrupted_2014-12-07-163010.emf | corrupted PMR_Object                                       |
| bad_corrupted_2014-12-07-235709.emf | corrupted EMREXTTEXTOUTW                                   |
| bad_corrupted_2014-12-09-002112.emf | corrupted EMREXTTEXTOUTA                                   |
| bad_corrupted_2014-12-13-105759.emf | corrupted EMR_HEADER (caused double free in object table)  |
| bad_corrupted_2014-12-13-132605.emf | corrupted EMR_HEADER (wrong nHandles)                      |
| bad_corrupted_2014-12-13-133727.emf | corrupted EMR_CREATEPEN (wrong ihPen)                      |
| bad_corrupted_2014-12-14-080539.emf | corrupted EMF_HEADER (bad nDescription)                    |
| bad_corrupted_2016-02-02-210039.emf | corrupted U_EMR_EXTTEXTOUTW (null string)                  |
| bad_corrupted_2016-02-02-220639.emf | null bitmap object                                         |
| bad_corrupted_2016-02-02-225639.emf | large width rel4 bitmap (cause hangs on EOL)               |
| bad_corrupted_2017-02-11-230400.emf | corruption of EMF+ Size field in HEADER                    |
| bad_corrupted_2017-02-12-010400.emf | weird EMF+ record with funky type and datasize 0           |
| bad_corrupted_2017-05-12-074000.emf | font index encoding with no font name set                  |
