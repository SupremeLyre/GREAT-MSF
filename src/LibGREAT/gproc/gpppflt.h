/**
 * @file         gpppflt.h
 * @author       GREAT-WHU (https://github.com/GREAT-WHU)
 * @brief        Matrix for NEQ,W,observ_equations
 * @version      1.0
 * @date         2024-08-29
 *
 * @copyright Copyright (c) 2024, Wuhan University. All rights reserved.
 *
 */
#ifndef GPPPFLT_H
#define GPPPFLT_H
#include "gexport/ExportLibGREAT.h"
#include <fstream>

#include "gio/gxml.h"
#include "gproc/gppp.h"
#include "gproc/gsppflt.h"

namespace gnut
{

class LibGREAT_LIBRARY_EXPORT t_gpppflt : public t_gppp, public t_gsppflt, public t_gxml
{
  public:
    /**@brief constructor */
    explicit t_gpppflt();
    explicit t_gpppflt(string mark, t_gsetbase *set);
    explicit t_gpppflt(string mark, t_gsetbase *set, t_spdlog spdlog);

    /**@brief destructor */
    virtual ~t_gpppflt();

  protected:
    /**
     * @brief Satelite position.
     * @param[in] t_gtime
     * @param[in] t_gsatdata
     * @return satPos
     */
    virtual int _satPos(t_gtime &, t_gsatdata &);

    /**
     * @brief Applying tides.
     * @param[in] _epoch  current epoch
     * @param[in] xRec    receiver crd
     * @return retval > 0,success; retval < 0,fail
     */
    virtual int _apply_tides(t_gtime &_epoch, t_gtriple &xRec);

    /**@brief Set output */
    virtual void _setOut();

    /**
     * @brief Save apriory coordinates to products.
     * @param[in] epo  current epoch
     * @param[in] X    parameter
     * @param[in] Q    covariance matrix
     */
    void _saveApr(t_gtime &epo, t_gallpar &X, const SymmetricMatrix &Q);

    /**
     * @brief Remove apriory coordinates from products.
     * @param[in] epo  current epoch
     */
    void _removeApr(t_gtime &epo);

    /**
     * @brief Update time for RDW processes.
     * @param[in] epo  current epoch
     */
    virtual void _timeUpdate(const t_gtime &epo);

    /**@brief reset parameters */
    void _reset_param();

    xml_node line;                ///< working line read from
    bool _read;                   ///< is read
    t_giof *_flt;                 ///< filter file
    string _kml_name;             ///< kml name
    bool _kml;                    ///< is kml
    bool _beg_end;                ///< processing direction
    t_randomwalk *_grdStoModel;   ///< tropo gradient models
    t_randomwalk *_ambStoModel;   ///< ambiguity models
    int _reset_amb;               ///< is reset ambiguity
    int _reset_par;               ///< is reset parameters
    t_gtriple xyz_standard;       ///< enu coord reference
    map<string, int> _glofrq_num; ///< glofrq number, set GLO frequency number
};

} // namespace gnut

#endif // GPPPFLT_H
