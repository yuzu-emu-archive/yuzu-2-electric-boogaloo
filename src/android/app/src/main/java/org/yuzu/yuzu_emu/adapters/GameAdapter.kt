package org.yuzu.yuzu_emu.adapters

import android.database.Cursor
import android.database.DataSetObserver
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.ContextCompat
import androidx.fragment.app.FragmentActivity
import androidx.recyclerview.widget.RecyclerView
import org.yuzu.yuzu_emu.R
import org.yuzu.yuzu_emu.activities.EmulationActivity.Companion.launch
import org.yuzu.yuzu_emu.model.GameDatabase
import org.yuzu.yuzu_emu.utils.Log
import org.yuzu.yuzu_emu.utils.PicassoUtils
import org.yuzu.yuzu_emu.viewholders.GameViewHolder
import java.util.*
import java.util.stream.Stream

/**
 * This adapter gets its information from a database Cursor. This fact, paired with the usage of
 * ContentProviders and Loaders, allows for efficient display of a limited view into a (possibly)
 * large dataset.
 */
class GameAdapter : RecyclerView.Adapter<GameViewHolder>(), View.OnClickListener {
    private var cursor: Cursor? = null
    private val observer: GameDataSetObserver?
    private var isDatasetValid = false

    /**
     * Initializes the adapter's observer, which watches for changes to the dataset. The adapter will
     * display no data until a Cursor is supplied by a CursorLoader.
     */
    init {
        observer = GameDataSetObserver()
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): GameViewHolder {
        // Create a new view.
        val gameCard = LayoutInflater.from(parent.context)
            .inflate(R.layout.card_game, parent, false)
        gameCard.setOnClickListener(this)

        // Use that view to create a ViewHolder.
        return GameViewHolder(gameCard)
    }

    override fun onBindViewHolder(holder: GameViewHolder, position: Int) {
        if (isDatasetValid) {
            if (cursor!!.moveToPosition(position)) {
                PicassoUtils.loadGameIcon(
                    holder.imageIcon,
                    cursor!!.getString(GameDatabase.GAME_COLUMN_PATH)
                )
                holder.textGameTitle.text =
                    cursor!!.getString(GameDatabase.GAME_COLUMN_TITLE)
                        .replace("[\\t\\n\\r]+".toRegex(), " ")
                holder.textGameCaption.text = cursor!!.getString(GameDatabase.GAME_COLUMN_CAPTION)

                // TODO These shouldn't be necessary once the move to a DB-based model is complete.
                holder.gameId = cursor!!.getString(GameDatabase.GAME_COLUMN_GAME_ID)
                holder.path = cursor!!.getString(GameDatabase.GAME_COLUMN_PATH)
                holder.title = cursor!!.getString(GameDatabase.GAME_COLUMN_TITLE)
                holder.description = cursor!!.getString(GameDatabase.GAME_COLUMN_DESCRIPTION)
                holder.regions = cursor!!.getString(GameDatabase.GAME_COLUMN_REGIONS)
                holder.company = cursor!!.getString(GameDatabase.GAME_COLUMN_CAPTION)
                val backgroundColorId =
                    if (isValidGame(holder.path!!)) R.color.view_background else R.color.view_disabled
                val itemView = holder.itemView
                itemView.setBackgroundColor(
                    ContextCompat.getColor(
                        itemView.context,
                        backgroundColorId
                    )
                )
            } else {
                Log.error("[GameAdapter] Can't bind view; Cursor is not valid.")
            }
        } else {
            Log.error("[GameAdapter] Can't bind view; dataset is not valid.")
        }
    }

    override fun getItemCount(): Int {
        if (isDatasetValid && cursor != null) {
            return cursor!!.count
        }
        Log.error("[GameAdapter] Dataset is not valid.")
        return 0
    }

    /**
     * Return the contents of the _id column for a given row.
     *
     * @param position The row for which Android wants an ID.
     * @return A valid ID from the database, or 0 if not available.
     */
    override fun getItemId(position: Int): Long {
        if (isDatasetValid && cursor != null) {
            if (cursor!!.moveToPosition(position)) {
                return cursor!!.getLong(GameDatabase.COLUMN_DB_ID)
            }
        }
        Log.error("[GameAdapter] Dataset is not valid.")
        return 0
    }

    /**
     * Tell Android whether or not each item in the dataset has a stable identifier.
     * Which it does, because it's a database, so always tell Android 'true'.
     *
     * @param hasStableIds ignored.
     */
    override fun setHasStableIds(hasStableIds: Boolean) {
        super.setHasStableIds(true)
    }

    /**
     * When a load is finished, call this to replace the existing data with the newly-loaded
     * data.
     *
     * @param cursor The newly-loaded Cursor.
     */
    fun swapCursor(cursor: Cursor) {
        // Sanity check.
        if (cursor === this.cursor) {
            return
        }

        // Before getting rid of the old cursor, disassociate it from the Observer.
        val oldCursor = this.cursor
        if (oldCursor != null && observer != null) {
            oldCursor.unregisterDataSetObserver(observer)
        }
        this.cursor = cursor
        isDatasetValid = if (this.cursor != null) {
            // Attempt to associate the new Cursor with the Observer.
            if (observer != null) {
                this.cursor!!.registerDataSetObserver(observer)
            }
            true
        } else {
            false
        }
        notifyDataSetChanged()
    }

    /**
     * Launches the game that was clicked on.
     *
     * @param view The card representing the game the user wants to play.
     */
    override fun onClick(view: View) {
        val holder = view.tag as GameViewHolder
        launch((view.context as FragmentActivity), holder.path, holder.title)
    }

    private fun isValidGame(path: String): Boolean {
        return Stream.of(".rar", ".zip", ".7z", ".torrent", ".tar", ".gz")
            .noneMatch { suffix: String? ->
                path.lowercase(Locale.getDefault()).endsWith(suffix!!)
            }
    }

    private inner class GameDataSetObserver : DataSetObserver() {
        override fun onChanged() {
            super.onChanged()
            isDatasetValid = true
            notifyDataSetChanged()
        }

        override fun onInvalidated() {
            super.onInvalidated()
            isDatasetValid = false
            notifyDataSetChanged()
        }
    }
}